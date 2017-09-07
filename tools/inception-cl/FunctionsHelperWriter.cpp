#include "FunctionsHelperWriter.h"

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
    case INIT_STACK:
      if (mod->getFunction("inception_init_stack") != NULL) return;

      // FNHInitStack(mod, before);
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
  std::vector<Type*> FuncTy_3_args;
  FunctionType* FuncTy_3 = FunctionType::get(
      /*Result=*/Type::getVoidTy(mod->getContext()),
      /*Params=*/FuncTy_3_args,
      /*isVarArg=*/false);

  PointerType* PointerTy_1 = PointerType::get(FuncTy_3, 0);

  std::vector<Type*> FuncTy_4_args;
  FunctionType* FuncTy_4 = FunctionType::get(
      /*Result=*/Type::getVoidTy(mod->getContext()),
      /*Params=*/FuncTy_4_args,
      /*isVarArg=*/true);

  // Function Declarations
  Function* func_inception_dump_registers =
      mod->getFunction("inception_dump_registers");
  if (!func_inception_dump_registers) {
    func_inception_dump_registers = Function::Create(
        /*Type=*/FuncTy_4,
        /*Linkage=*/GlobalValue::ExternalLinkage,
        /*Name=*/"inception_dump_registers", mod);  // (external, no body)
    func_inception_dump_registers->setCallingConv(CallingConv::C);
  }
  AttributeSet func_inception_dump_registers_PAL;
  {
    SmallVector<AttributeSet, 4> Attrs;
    AttributeSet PAS;
    {
      AttrBuilder B;
      PAS = AttributeSet::get(mod->getContext(), ~0U, B);
    }

    Attrs.push_back(PAS);
    func_inception_dump_registers_PAL =
        AttributeSet::get(mod->getContext(), Attrs);
  }
  func_inception_dump_registers->setAttributes(
      func_inception_dump_registers_PAL);

  // Constant Definitions
  Constant* const_ptr_7 = ConstantExpr::getCast(
      Instruction::BitCast, func_inception_dump_registers, PointerTy_1);
  std::vector<Value*> params;

  // Function: main (func_main)
  {
    CallInst* void_10 = CallInst::Create(const_ptr_7, params, "", inst);
    void_10->setCallingConv(CallingConv::C);
    void_10->setTailCall(false);
  }
}

void FunctionsHelperWriter::FNHInitStack(llvm::Module* mod,
                                         llvm::Instruction* inst) {

}
