#include "IRMerger.h"

#include "FunctionCleaner.h"
#include "FunctionsHelperWriter.h"
#include "StackAllocator.h"

extern bool nameLookupAddr(StringRef funcName, uint64_t& Address);

namespace fracture {

bool IRMerger::first_call = true;

std::map<std::string, SDNode*> IRMerger::registersNodes = {};

IRMerger::IRMerger(Decompiler* P_DEC) : DEC(P_DEC) {
  // RegMap.grow(DEC->getDisassembler()
  //                 ->getMCDirector()
  //                 ->getMCRegisterInfo()
  //                 ->getNumRegs());
}

IRMerger::~IRMerger() {}

void IRMerger::Run(llvm::StringRef name) {
  Module* mod = DEC->getModule();

  if (IRMerger::first_call) {
    IRMerger::first_call = false;
    // Init the stack
    StackAllocator::Allocate(mod);
  }

  Function* fct = mod->getFunction(name);

  FunctionCleaner::Clean(fct);

  if (fct->empty()) {
    BasicBlock::Create(mod->getContext(), "entry", fct);
    fct->addFnAttr("Empty", "True");
  }

  WriteABIPrologue(fct);

  Decompile(name);

  WriteABIEpilogue(fct);

  FunctionsHelperWriter::Write(END, DUMP_REGISTERS, mod);
  FunctionsHelperWriter::Write(BEGIN, INIT_STACK, mod);
}

void IRMerger::Decompile(llvm::StringRef name) {
  uint64_t Address;

  if (nameLookupAddr(name, Address) == false) {
    return;
  }

  formatted_raw_ostream Out(outs(), false);

  DEC->decompile(Address);
}

void IRMerger::WriteABIEpilogue(llvm::Function* fct) {
  // TRANSFORM NEW RETURN TO A RETURN WITH THE RIGHT TYPE
  unsigned int ret_counter = 0;
  ReturnInst* ret;

  Type* FType = fct->getReturnType();

  // void return has been write by Lifter
  if (FType->isVoidTy()) {
    return;
  }

  auto bb = fct->begin();

  for (bb; bb != fct->end(); bb++) {
    Instruction* inst = (*bb).begin();

    while (inst != (*bb).end() || inst == nullptr) {
      auto next = inst->getNextNode();

      if ((ret = dyn_cast<ReturnInst>(inst)) != NULL) {
        IRBuilder<>* IRB = new IRBuilder<>(inst);

        Value* Reg = getReg(StringRef("R0"));

        StringRef name("R0_RET" + std::to_string(ret_counter));

        Instruction* Res = new LoadInst(Reg, name, "", inst);

        // caast ptr to int to ptr to correct type if necessary
        if (FType->isPointerTy()) {
          Res = new IntToPtrInst(Res, FType, "", inst);
        } else if (FType->isIntegerTy() && FType->getIntegerBitWidth() < 32) {
          Res = new TruncInst(Res, FType, "", inst);
        }

        Res = ReturnInst::Create(*(DEC->getContext()), Res);

        llvm::ReplaceInstWithInst(inst, Res);
        ret_counter++;
      }
      inst = next;
    }
  }
}  // namespace fracture

void IRMerger::WriteABIPrologue(llvm::Function* fct) {
  BasicBlock& new_entry = fct->getEntryBlock();

  IRBuilder<>* IRB = new IRBuilder<>(&new_entry);

  Module* mod = DEC->getModule();

  uint8_t reg_counter = 0;

  for (auto arg = fct->arg_begin(); arg != fct->arg_end(); arg++) {
    Value* x = arg;

    std::string reg_name = "R" + std::to_string(reg_counter);

    Value* reg = getReg(StringRef(reg_name));

    if (x->getType()->isPointerTy()) {
      x = IRB->CreatePtrToInt(x, IntegerType::get(mod->getContext(), 32));
    }

    if (x->getType()->isIntegerTy() &&
        x->getType()->getIntegerBitWidth() < 32) {
      x = IRB->CreateZExt(x, IntegerType::get(mod->getContext(), 32));
    }

    Instruction* Res = IRB->CreateStore(x, reg);

    reg_counter++;
  }
}

}  // namespace fracture
