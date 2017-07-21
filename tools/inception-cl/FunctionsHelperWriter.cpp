#include "FunctionsHelperWriter.h"

void FunctionsHelperWriter::Write(FHW_POSITION position, FUNCTION_HELPER type,
                                  llvm::Module* mod,
                                  llvm::Instruction* before) {
  switch (position) {
    case BEGIN:
      before = GetBegin(mod);
      break;
    case END:
      before = GetLast(mod);
      break;
    case NONE:
      if (before == NULL) return;
  }

  switch (type) {
    case INIT_STACK:
      if (mod->getFunction("inception_init_stack") != NULL) return;

      FNHInitStack(mod, before);
      break;
    case DUMP_REGISTERS:
      if (mod->getFunction("inception_dump_registers") != NULL) return;

      FNHDumpRegisters(mod, before);
      break;
  }
}

llvm::Instruction* FunctionsHelperWriter::GetBegin(llvm::Module* mod) {
  Instruction* ptr;

  for (auto fct = mod->getFunctionList().begin();
       fct != mod->getFunctionList().end(); fct++)
    for (auto bb_i = fct->getBasicBlockList().begin();
         bb_i != fct->getBasicBlockList().end(); bb_i++) {
      BasicBlock& old_bb = *bb_i;
      for (auto int_i = old_bb.begin(); int_i != old_bb.end(); int_i++)
        return int_i;
    }
}

llvm::Instruction* FunctionsHelperWriter::GetLast(llvm::Module* mod) {
  Instruction* ptr;

  for (auto fct = mod->getFunctionList().begin();
       fct != mod->getFunctionList().end(); fct++)
    for (auto bb_i = fct->getBasicBlockList().begin();
         bb_i != fct->getBasicBlockList().end(); bb_i++) {
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
  ArrayType* ArrayTy_0 =
      ArrayType::get(IntegerType::get(mod->getContext(), 8), 8200);

  std::vector<Type*> FuncTy_2_args;
  FunctionType* FuncTy_2 = FunctionType::get(
      /*Result=*/Type::getVoidTy(mod->getContext()),
      /*Params=*/FuncTy_2_args,
      /*isVarArg=*/false);

  // Function Declarations
  Function* func___INCEPTION_INIT_STACK =
      mod->getFunction("inception_init_stack");
  if (!func___INCEPTION_INIT_STACK) {
    func___INCEPTION_INIT_STACK = Function::Create(
        /*Type=*/FuncTy_2,
        /*Linkage=*/GlobalValue::ExternalLinkage,
        /*Name=*/"inception_init_stack", mod);
    func___INCEPTION_INIT_STACK->setCallingConv(67);
  }
  AttributeSet func___INCEPTION_INIT_STACK_PAL;
  {
    SmallVector<AttributeSet, 4> Attrs;
    AttributeSet PAS;
    {
      AttrBuilder B;
      B.addAttribute(Attribute::NoUnwind);
      PAS = AttributeSet::get(mod->getContext(), ~0U, B);
    }

    Attrs.push_back(PAS);
    func___INCEPTION_INIT_STACK_PAL =
        AttributeSet::get(mod->getContext(), Attrs);
  }
  func___INCEPTION_INIT_STACK->setAttributes(func___INCEPTION_INIT_STACK_PAL);

  // Global Variable Declarations

  GlobalVariable* gvar_array_STACK =
      new GlobalVariable(/*Module=*/*mod,
                         /*Type=*/ArrayTy_0,
                         /*isConstant=*/false,
                         /*Linkage=*/GlobalValue::ExternalLinkage,
                         /*Initializer=*/0,  // has initializer, specified below
                         /*Name=*/"STACK");
  gvar_array_STACK->setAlignment(1);

  // Constant Definitions
  ConstantAggregateZero* const_array_5 = ConstantAggregateZero::get(ArrayTy_0);
  ConstantInt* const_int32_6 =
      ConstantInt::get(mod->getContext(), APInt(32, StringRef("1"), 10));
  ConstantInt* const_int32_7 =
      ConstantInt::get(mod->getContext(), APInt(32, StringRef("0"), 10));
  ConstantInt* const_int32_8 =
      ConstantInt::get(mod->getContext(), APInt(32, StringRef("8200"), 10));

  // Global Variable Definitions
  gvar_array_STACK->setInitializer(const_array_5);

  // Function Definitions

  // Function: __INCEPTION_INIT_STACK (func___INCEPTION_INIT_STACK)
  {
    BasicBlock* label_entry = BasicBlock::Create(
        mod->getContext(), "entry", func___INCEPTION_INIT_STACK, 0);
    BasicBlock* label_for_cond = BasicBlock::Create(
        mod->getContext(), "for.cond", func___INCEPTION_INIT_STACK, 0);
    BasicBlock* label_for_body = BasicBlock::Create(
        mod->getContext(), "for.body", func___INCEPTION_INIT_STACK, 0);
    BasicBlock* label_for_inc = BasicBlock::Create(
        mod->getContext(), "for.inc", func___INCEPTION_INIT_STACK, 0);
    BasicBlock* label_for_end = BasicBlock::Create(
        mod->getContext(), "for.end", func___INCEPTION_INIT_STACK, 0);

    // Block entry (label_entry)
    AllocaInst* ptr_i = new AllocaInst(IntegerType::get(mod->getContext(), 32),
                                       "i", label_entry);
    ptr_i->setAlignment(4);
    StoreInst* void_9 = new StoreInst(const_int32_7, ptr_i, false, label_entry);
    void_9->setAlignment(4);
    StoreInst* void_10 =
        new StoreInst(const_int32_7, ptr_i, false, label_entry);
    void_10->setAlignment(4);
    BranchInst::Create(label_for_cond, label_entry);

    // Block for.cond (label_for_cond)
    LoadInst* int32_12 = new LoadInst(ptr_i, "", false, label_for_cond);
    int32_12->setAlignment(4);
    ICmpInst* int1_cmp = new ICmpInst(*label_for_cond, ICmpInst::ICMP_ULT,
                                      int32_12, const_int32_8, "cmp");
    BranchInst::Create(label_for_body, label_for_end, int1_cmp, label_for_cond);

    // Block for.body (label_for_body)
    LoadInst* int32_14 = new LoadInst(ptr_i, "", false, label_for_body);
    int32_14->setAlignment(4);
    CastInst* int8_conv =
        new TruncInst(int32_14, IntegerType::get(mod->getContext(), 8), "conv",
                      label_for_body);
    LoadInst* int32_15 = new LoadInst(ptr_i, "", false, label_for_body);
    int32_15->setAlignment(4);
    std::vector<Value*> ptr_arrayidx_indices;
    ptr_arrayidx_indices.push_back(const_int32_7);
    ptr_arrayidx_indices.push_back(int32_15);
    Instruction* ptr_arrayidx = GetElementPtrInst::Create(
        gvar_array_STACK, ptr_arrayidx_indices, "arrayidx", label_for_body);
    StoreInst* void_16 =
        new StoreInst(int8_conv, ptr_arrayidx, false, label_for_body);
    void_16->setAlignment(1);
    BranchInst::Create(label_for_inc, label_for_body);

    // Block for.inc (label_for_inc)
    LoadInst* int32_18 = new LoadInst(ptr_i, "", false, label_for_inc);
    int32_18->setAlignment(4);
    BinaryOperator* int32_inc = BinaryOperator::Create(
        Instruction::Add, int32_18, const_int32_6, "inc", label_for_inc);
    StoreInst* void_19 = new StoreInst(int32_inc, ptr_i, false, label_for_inc);
    void_19->setAlignment(4);
    BranchInst::Create(label_for_cond, label_for_inc);

    // Block for.end (label_for_end)
    ReturnInst::Create(mod->getContext(), label_for_end);

    PointerType* PointerTy_1 = PointerType::get(FuncTy_2, 0);

    // Constant Definitions
    Constant* const_ptr_7 = ConstantExpr::getCast(
        Instruction::BitCast, func___INCEPTION_INIT_STACK, PointerTy_1);
    std::vector<Value*> params;
    // Function: main (func_main)
    {
      CallInst* void_10;

      const ReturnInst* ret = dyn_cast<ReturnInst>(inst);
      if (ret != NULL) {
        void_10 = CallInst::Create(const_ptr_7, params, "");

        void_10->insertBefore(inst);
      } else
        void_10 = CallInst::Create(const_ptr_7, params, "", inst);

      void_10->setCallingConv(CallingConv::C);
      void_10->setTailCall(false);
    }
  }
}
