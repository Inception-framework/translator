#include "IRMerger.h"

extern bool nameLookupAddr(StringRef funcName, uint64_t& Address);

namespace fracture {

bool IRMerger::first_call = true;

std::map<std::string, SDNode*> IRMerger::registersNodes = {};

IRMerger::IRMerger(Decompiler* P_DEC, std::string new_function_name)
    : DEC(P_DEC) {
  RegMap.grow(DEC->getDisassembler()
                  ->getMCDirector()
                  ->getMCRegisterInfo()
                  ->getNumRegs());

  function_name = new StringRef(new_function_name);
}

IRMerger::~IRMerger() {
  marked_old_instructions.clear();

  marked_old_basicblocks.clear();
}

void IRMerger::SetNewFunction(std::string new_function_name) {
  function_name = new StringRef(new_function_name);

  marked_old_instructions.clear();

  marked_old_basicblocks.clear();
}

void IRMerger::Run() {
  if (IRMerger::first_call) {
    IRMerger::first_call = false;

    // CreateADDCarryHelper();
  }

  bool empty = false;

  fct = DEC->getModule()->getFunction(*function_name);

  if (fct->empty()) empty = true;

  if (!empty) {
    entry_bb = &(fct->getEntryBlock());

    MarkOldInstructions();
  } else {
    LLVMContext context;

    entry_bb = BasicBlock::Create(context, "entry", fct);

    fct->addFnAttr("Empty", "True");
  }

  MapArgsToRegs();

  Function* new_function = Decompile();
  if (new_function == NULL) return;

  if (!empty) {
    RemoveUseless();
  }

  // DEC->getModule()->dump();
}

void IRMerger::InsertDump(llvm::Instruction *inst) {

  Module* mod = DEC->getModule();

  std::vector<Type*>FuncTy_3_args;
  FunctionType* FuncTy_3 = FunctionType::get(
   /*Result=*/Type::getVoidTy(mod->getContext()),
   /*Params=*/FuncTy_3_args,
   /*isVarArg=*/false);

  PointerType* PointerTy_1 = PointerType::get(FuncTy_3, 0);

  std::vector<Type*>FuncTy_4_args;
  FunctionType* FuncTy_4 = FunctionType::get(
   /*Result=*/Type::getVoidTy(mod->getContext()),
   /*Params=*/FuncTy_4_args,
   /*isVarArg=*/true);


  // Function Declarations
  Function* func_inception_dump_registers = mod->getFunction("inception_dump_registers");
  if (!func_inception_dump_registers) {
  func_inception_dump_registers = Function::Create(
   /*Type=*/FuncTy_4,
   /*Linkage=*/GlobalValue::ExternalLinkage,
   /*Name=*/"inception_dump_registers", mod); // (external, no body)
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
   func_inception_dump_registers_PAL = AttributeSet::get(mod->getContext(), Attrs);

  }
  func_inception_dump_registers->setAttributes(func_inception_dump_registers_PAL);

  // Constant Definitions
  Constant* const_ptr_7 = ConstantExpr::getCast(Instruction::BitCast, func_inception_dump_registers, PointerTy_1);
  std::vector<Value*> params;

  // Function: main (func_main)
  {
   CallInst* void_10 = CallInst::Create(const_ptr_7, params, "", inst);
   void_10->setCallingConv(CallingConv::C);
   void_10->setTailCall(false);
  }
}

void IRMerger::CreateADDCarryHelper() {
  Module* mod = DEC->getModule();

  std::vector<Type*> FuncTy_args;
  FuncTy_args.push_back(IntegerType::get(mod->getContext(), 32));
  FuncTy_args.push_back(IntegerType::get(mod->getContext(), 32));
  FunctionType* FuncTy = FunctionType::get(
      /*Result=*/IntegerType::get(mod->getContext(), 32),
      /*Params=*/FuncTy_args,
      /*isVarArg=*/false);

  Function* func__Z14ADD_WITH_CARRYii =
      mod->getFunction("_Z14ADD_WITH_CARRYii");
  if (!func__Z14ADD_WITH_CARRYii) {
    func__Z14ADD_WITH_CARRYii = Function::Create(
        /*Type=*/FuncTy,
        /*Linkage=*/GlobalValue::ExternalLinkage,
        /*Name=*/"_Z14ADD_WITH_CARRYii", mod);
    func__Z14ADD_WITH_CARRYii->setCallingConv(67);
  }

  GlobalVariable* gvar_int32_CPSR = mod->getGlobalVariable("CPSR");
  if (gvar_int32_CPSR == NULL) {

    Type* Ty = IntegerType::get(mod->getContext(), 32);

    Constant *Initializer = Constant::getNullValue(Ty);

    gvar_int32_CPSR = new GlobalVariable(*mod, // Module
                             Ty,                // Type
                             false,             // isConstant
                             GlobalValue::ExternalLinkage,
                             Initializer,
                             "CPSR");
  }

  Function::arg_iterator args = func__Z14ADD_WITH_CARRYii->arg_begin();

  Value* int32_a = args++;
  int32_a->setName("a");
  Value* int32_b = args++;
  int32_b->setName("b");

  ConstantInt* carry =
      ConstantInt::get(mod->getContext(), APInt(32, StringRef("1"), 10));

  BasicBlock* label_entry = BasicBlock::Create(mod->getContext(), "entry",
                                               func__Z14ADD_WITH_CARRYii, 0);
  BasicBlock* label_if_then = BasicBlock::Create(mod->getContext(), "if.then",
                                                 func__Z14ADD_WITH_CARRYii, 0);
  BasicBlock* label_if_end = BasicBlock::Create(mod->getContext(), "if.end",
                                                func__Z14ADD_WITH_CARRYii, 0);
  BasicBlock* label_return = BasicBlock::Create(mod->getContext(), "return",
                                                func__Z14ADD_WITH_CARRYii, 0);

  // Block entry (label_entry)
  AllocaInst* ptr_a_addr = new AllocaInst(
      IntegerType::get(mod->getContext(), 32), "a.addr", label_entry);

  AllocaInst* ptr_b_addr = new AllocaInst(
      IntegerType::get(mod->getContext(), 32), "b.addr", label_entry);

  AllocaInst* ptr_c_addr =
      new AllocaInst(IntegerType::get(mod->getContext(), 32), "c", label_entry);

  int32_a = new LoadInst(ptr_a_addr, "", false, label_entry);

  int32_b = new LoadInst(ptr_b_addr, "", false, label_entry);

  BinaryOperator* a_add_b = BinaryOperator::Create(Instruction::Add, int32_a,
                                                   int32_b, "add", label_entry);
  LoadInst* cpsr = new LoadInst(gvar_int32_CPSR, "", false, label_entry);

  BinaryOperator* a_add_b_add_cpsr = BinaryOperator::Create(
      Instruction::Add, a_add_b, cpsr, "add1", label_entry);
  new StoreInst(a_add_b_add_cpsr, ptr_c_addr, false, label_entry);

  ICmpInst* cmp = new ICmpInst(*label_entry, ICmpInst::ICMP_SGT, int32_a,
                               a_add_b_add_cpsr, "cmp");
  BranchInst::Create(label_if_then, label_if_end, cmp, label_entry);

  // Block if.then (label_if_then)
  new StoreInst(carry, gvar_int32_CPSR, false, label_if_then);
  BranchInst::Create(label_if_end, label_if_then);

  // Block if.end (label_if_end)
  ReturnInst::Create(mod->getContext(), a_add_b_add_cpsr, label_if_end);

  // Block return (label_return)
  ReturnInst::Create(mod->getContext(), a_add_b_add_cpsr, label_return);
}

Function* IRMerger::Decompile() {
  uint64_t Address;

  outs() << "[Inception] The funcion " << *function_name;
  outs() << " needs to be replaced...\n";

  if (nameLookupAddr(*function_name, Address) == false) {
    return NULL;
  }

  DEC->setViewMCDAGs(true);
  DEC->setViewIRDAGs(true);

  formatted_raw_ostream Out(outs(), false);

  DEC->decompile(Address);

  outs() << "-----------New IR Code --------------------\n";
  DEC->printInstructions(Out, Address);
  outs() << "-------------------------------------------\n";

  return DEC->getModule()->getFunction(*function_name);
}

void IRMerger::MarkOldInstructions() {
  for (auto bb_i = fct->begin(); bb_i != fct->end(); bb_i++) {
    BasicBlock& old_bb = *bb_i;

    for (auto int_i = old_bb.begin(); int_i != old_bb.end(); int_i++) {
      Instruction& old_inst = *int_i;

      if (&old_inst != NULL) {
        const UnreachableInst* ui = dyn_cast<UnreachableInst>(&old_inst);
        const ReturnInst* ret = dyn_cast<ReturnInst>(&old_inst);

        if (ui != NULL) {
          marked_old_binstructions.push_back(&old_inst);
        } else if (ret != NULL) {
          marked_old_binstructions.push_back(&old_inst);
        } else
          marked_old_instructions.push_back(&old_inst);
      }
    }  // END INST LOOP

    if (&old_bb != entry_bb) {
      marked_old_basicblocks.push_back(&old_bb);
      // old_bb.dropAllReferences();
      // old_bb.removeFromParent();
      // old_bb.insertInto (fct);
    }
  }  // END BB LOOP
}

void IRMerger::RemoveUseless() {
  BasicBlock* last_bb = NULL;

  for (auto bb_i = fct->begin(); bb_i != fct->end(); bb_i++) last_bb = bb_i;

  unsigned int ret_counter = 0;

  for (auto bb_i = fct->begin(); bb_i != fct->end(); bb_i++) {
    BasicBlock& old_bb = *bb_i;
    for (auto int_i = old_bb.begin(); int_i != old_bb.end(); int_i++) {
      Instruction& old_inst = *int_i;

      SmallVector<std::pair<unsigned, MDNode*>, 100> Metadata;

      int_i->getAllMetadata(Metadata);

      for (unsigned i = 0, n = Metadata.size(); i < n; ++i) {
        unsigned Kind = Metadata[i].first;

        int_i->setMetadata(Kind, nullptr);
      }
      Metadata.clear();

      if (&old_inst != NULL) {
        const ReturnInst* ret = dyn_cast<ReturnInst>(&old_inst);

        if (ret != NULL) {
          marked_old_binstructions.push_back(&old_inst);

          IRBuilder<>* IRB = new IRBuilder<>(&old_inst);

          Type* FType = fct->getReturnType();

          if (FType->isVoidTy()) {

            InsertDump(&old_inst);

            IRB->CreateRetVoid();

            continue;
          }

          InsertDump(&old_inst);

          Value* Reg = DEC->getModule()->getGlobalVariable("R0");
          if (Reg == NULL) {

            Type* Ty = IntegerType::get(DEC->getModule()->getContext(), 32);

            Constant* Initializer = Constant::getNullValue(Ty);

            GlobalVariable* gvar_i32 =
                new GlobalVariable(*DEC->getModule(),  // Module
                                   Ty,                 // Type
                                   false,              // isConstant
                                   GlobalValue::CommonLinkage, Initializer, "R0");

           Reg = gvar_i32;

            // outs() << "MISSING REGISTER R0 TO CREATE RETURN INSTRUCTION \n\n";
            // return;
          }

          if (FType->isPointerTy()) IRB->CreateRet(Reg);

          if (FType->isIntegerTy()) {
            std::string ret_name = "R0_RET" + std::to_string(ret_counter);
            StringRef ReturnName(ret_name);

            Instruction* Res = IRB->CreateLoad(Reg, ReturnName);

            IRB->CreateRet(Res);

            ret_counter++;
          }
        }
      }
    }
  }

  for (auto& inst : marked_old_instructions) {
    RemoveInstruction(inst);
  }

  for (auto& inst : marked_old_binstructions) {
    RemoveInstruction(inst);
  }

  for (auto& bb : marked_old_basicblocks) {

    bb->dropAllReferences();
    // bb->removeFromParent();
    bb->eraseFromParent();
  }
}

void IRMerger::RemoveInstruction(llvm::Instruction* instruction) {
  SmallVector<std::pair<unsigned, MDNode*>, 100> Metadata;

  instruction->getAllMetadata(Metadata);

  for (unsigned i = 0, n = Metadata.size(); i < n; ++i) {
    unsigned Kind = Metadata[i].first;

    instruction->setMetadata(Kind, nullptr);
  }
  Metadata.clear();

  outs() << "Removing : " << *instruction << "\n";
  instruction->dropAllReferences();
  // instruction->removeFromParent();
  instruction->eraseFromParent();
}

void IRMerger::MapArgsToRegs() {
  // outs() << "=========MAP ARGS===========\n\n";

  BasicBlock& new_entry = fct->getEntryBlock();

  IRBuilder<>* IRB = new IRBuilder<>(&new_entry);

  uint8_t reg_counter = 0;
  for (auto arg = fct->arg_begin(); arg != fct->arg_end(); arg++) {
    Value* x = arg;

    std::string reg_name = "R" + std::to_string(reg_counter);

    Value* Reg = DEC->getModule()->getGlobalVariable(reg_name);
    if (Reg == NULL) {
      // ConstantInt* Ty = ConstantInt::get(DEC->getModule()->getContext(),
      // APInt(32,0));
      // PointerType::get(IntegerType::get( DEC->getModule()->getContext(), 32),
      // 0);
      // std::map<std::string, SDNode*>::iterator it;
      // it = registersNodes.find(reg_name);
      // if (it != registersNodes.end()) {
      //
      //   SDNode* node = it->second;
      //
      //   const RegisterSDNode *R = dyn_cast<RegisterSDNode>(node);
      //
      //   Ty = R->getValueType(0).getTypeForEVT(getGlobalContext());
      // }
      // else
      Type* Ty = IntegerType::get(DEC->getModule()->getContext(), 32);

      Constant* Initializer = Constant::getNullValue(Ty);

      GlobalVariable* gvar_i32 =
          new GlobalVariable(*DEC->getModule(),  // Module
                             Ty,                 // Type
                             false,              // isConstant
                             GlobalValue::CommonLinkage, Initializer, reg_name);
      // gvar_i32->setAlignment(4);

      RegMap[reg_counter] = gvar_i32;

      Reg = gvar_i32;

      // outs() << "\n[Inception]\tAdding new register " << reg_name << "\n";
    }
    // Reg->dump();

    // if (!Addr->getType()->isPointerTy()) {

    // StringRef AddrName(reg_name+"_FARG_P");

    // Addr = IRB->CreateIntToPtr(Addr, Addr->getType()->getPointerTo(),
    // AddrName); outs() << "\nCast "; Addr->dump();
    // }

    // if ( Reg->getType() != x->getType() ) {
    //   outs() << "Solving type issue \n\n";
    //
    //   outs() << "\nRegType "; Reg->getType()->dump();
    //   outs() << "\nArgType "; x->getType()->dump();
    //   outs() << "\nElementType "; x->getElementType()->dump();
    //
    //   else
    //     outs() << "Unknown type ....\n\n";
    // }

    if (x->getType()->isPointerTy()) {
      continue;

      x = IRB->CreateLoad(x);
      // outs() << "\nGet Ptr value ";
      // x->dump();

      if (Reg->getType() != x->getType()) {
        x = IRB->CreateBitCast(x, Reg->getType());
        // outs() << "\nBitcast ";
        // x->dump();
      }
      // x = IRB->CreatePtrToInt(x, x->getType()->getPointerTo());
    }

    // outs() << "\nRegType ";
    // Reg->getType()->dump();
    // outs() << "\nArgType ";
    // x->getType()->dump();

    Instruction* Res = IRB->CreateStore(x, Reg);
    Res->dump();
    // outs() << "==========================\n\n";

    reg_counter++;
  }
}

}  // namespace fracture
