#include "IRMerger.h"

#include "FunctionCleaner.h"
#include "FunctionsHelperWriter.h"
#include "SectionsWriter.h"
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

  fct->dump();
  Decompile(name);

  WriteABIEpilogue(fct);

  Function* main = DEC->getModule()->getFunction("main");
  FunctionsHelperWriter::Write(END, DUMP_REGISTERS, mod, main);
  FunctionsHelperWriter::Write(BEGIN, INIT_STACK, mod, main);

  SectionsWriter::WriteSection(".data" , DEC->getDisassembler(), mod);
  SectionsWriter::WriteSection(".bss" , DEC->getDisassembler(), mod);
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
    Value* Res = NULL;

    Value* Reg = getReg(StringRef("R" + std::to_string(reg_counter)));

    if (arg->getType()->isPointerTy()) {
      Res = IRB->CreatePtrToInt(arg, IntegerType::get(mod->getContext(), 32));
    }

    if (arg->getType()->isIntegerTy()) {
      Res = IRB->CreateZExt(arg, IntegerType::get(mod->getContext(), 32));
    }

    if (arg->getType()->isArrayTy()) {
      for (int i = 0; i < arg->getType()->getArrayNumElements(); i++) {
        if (i != 0) reg_counter++;
        Reg = getReg(StringRef("R" + std::to_string(reg_counter)));
        Value* element = IRB->CreateExtractValue(arg, i);
        Res = IRB->CreateStore(element, Reg);
      }
      continue;
    }

    IRB->CreateStore(Res, Reg);
    reg_counter++;
  }
}

}  // namespace fracture
