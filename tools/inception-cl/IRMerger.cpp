#include "IRMerger.h"

#include "FunctionCleaner.h"
#include "Utils/Builder.h"
#include "Utils/IContext.h"

extern bool nameLookupAddr(StringRef funcName, uint64_t& Address);

namespace fracture {

IRMerger::IRMerger(Decompiler* P_DEC) : DEC(P_DEC) {}

IRMerger::~IRMerger() {}

void IRMerger::Run(llvm::StringRef name) {
  Module* mod = IContext::Mod;

  Function* fct = mod->getFunction(name);

  FunctionCleaner::Clean(fct);

  if (fct->empty()) {
    BasicBlock::Create(mod->getContext(), "entry", fct);
    fct->addFnAttr("Empty", "True");
  }

  WriteABIPrologue(fct);

  Decompile(name);

  WriteABIEpilogue(fct);
}

void IRMerger::Decompile(llvm::StringRef name) {
  uint64_t address;

  Disassembler* DIS = (Disassembler*)DEC->getDisassembler();

  if (nameLookupAddr(name, address, DIS) == false) {
    inception_error("Wrong symbol table, function %s undefined", name);
  }

  std::string fileName = name.str() + std::string(".dis");

  DIS->setDisassFileNameAndAddr(fileName, address);

  std::error_code ErrorInfo;
  raw_fd_ostream FOut(fileName, ErrorInfo, sys::fs::OpenFlags::F_RW);
  formatted_raw_ostream Out(FOut, false);

  StringRef SectionName;
  object::SectionRef Section = DIS->getSectionByAddress(address);
  DIS->setSection(Section);

  if (DIS->printInstructions(Out, address, 0, false) == 0)
    inception_warning("Cannot create disassembled file for %s", name);

  DEC->decompile(address);
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

  for (auto bb = fct->begin(); bb != fct->end(); bb++) {
    Instruction* inst = (*bb).begin();

    while (inst != (*bb).end() || inst == nullptr) {
      auto next = inst->getNextNode();

      if ((ret = dyn_cast<ReturnInst>(inst)) != NULL) {
        Value* reg = Reg(StringRef("R0"));

        StringRef name("R0_RET" + std::to_string(ret_counter));

        Instruction* Res = new LoadInst(reg, name, "", inst);

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

    Value* reg = Reg(StringRef("R" + std::to_string(reg_counter)));

    if (arg->getType()->isPointerTy()) {
      Res = IRB->CreatePtrToInt(arg, IntegerType::get(mod->getContext(), 32));
    }

    if (arg->getType()->isIntegerTy()) {
      Res = IRB->CreateZExt(arg, IntegerType::get(mod->getContext(), 32));
    }

    if (arg->getType()->isArrayTy()) {
      for (uint64_t i = 0; i < arg->getType()->getArrayNumElements(); i++) {
        if (i != 0) reg_counter++;
        reg = Reg(StringRef("R" + std::to_string(reg_counter)));
        Value* element = IRB->CreateExtractValue(arg, i);
        Res = IRB->CreateStore(element, reg);
      }
      continue;
    }

    IRB->CreateStore(Res, reg);
    reg_counter++;
  }
}

}  // namespace fracture
