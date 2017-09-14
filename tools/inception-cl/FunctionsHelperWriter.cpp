#include "FunctionsHelperWriter.h"

#include "Utils/Builder.h"

void FunctionsHelperWriter::Write(FHW_POSITION position, FUNCTION_HELPER type,
                                  llvm::Module* mod, llvm::Function* func,
                                  llvm::Instruction* before) {
  switch (position) {
    case BEGIN:
      before = GetBegin(func);
      break;
    case END:
      before = GetLast(func);
      break;
    case NONE:
      if (before == NULL) return;
  }

  switch (type) {
    case DUMP_STACK:
      if (mod->getFunction("inception_dump_stack") != NULL) return;

      FNHDumpStack(mod, before);
      break;
    case DUMP_REGISTERS:
      if (mod->getFunction("inception_dump_registers") != NULL) return;

      FNHDumpRegisters(mod, before);
      break;
  }
}

llvm::Instruction* FunctionsHelperWriter::GetBegin(llvm::Function* func) {
  for (auto bb_i = func->getBasicBlockList().begin();
       bb_i != func->getBasicBlockList().end(); bb_i++) {
    BasicBlock& old_bb = *bb_i;
    for (auto int_i = old_bb.begin(); int_i != old_bb.end(); int_i++)
      return int_i;
  }

  return NULL;
}

llvm::Instruction* FunctionsHelperWriter::GetLast(llvm::Function* func) {
  Instruction* ptr;

  for (auto bb_i = func->getBasicBlockList().begin();
       bb_i != func->getBasicBlockList().end(); bb_i++) {
    BasicBlock& old_bb = *bb_i;
    for (auto int_i = old_bb.begin(); int_i != old_bb.end(); int_i++)
      ptr = int_i;
  }
  return ptr;
}

void FunctionsHelperWriter::FNHDumpRegisters(llvm::Module* mod,
                                             llvm::Instruction* inst) {
  Constant* const_ptr = GetVoidFunctionPointer("inception_dump_registers");

  std::vector<Value*> params;

  // Function: main (func_main)
  {
    CallInst* void_10 = CallInst::Create(const_ptr, params, "", inst);
    void_10->setCallingConv(CallingConv::C);
    void_10->setTailCall(false);
  }
}

void FunctionsHelperWriter::FNHDumpStack(llvm::Module* mod,
                                         llvm::Instruction* inst) {
  Constant* func_ptr = GetVoidFunctionPointer("inception_dump_stack_range");

  IRBuilder<>* IRB = new IRBuilder<>(inst);

  std::vector<Value*> params;

  Value* val = ReadReg(Reg("SP"), IRB, 32);
  params.push_back(val);
  val = IRB->CreateSub(val, getConstant(30));
  params.push_back(val);

  // Function: main (func_main)
  {
    CallInst* call = CallInst::Create(func_ptr, params, "", inst);
    call->setCallingConv(CallingConv::C);
    call->setTailCall(false);
  }
}
