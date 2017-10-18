#include "FunctionsHelperWriter.h"

#include "Utils/Builder.h"
#include "Utils/IContext.h"

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
      break;
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
    case INTERRUPT_PROLOGUE:
      if (mod->getFunction("inception_interrupt_prologue") != NULL) return;

      FNHInterruptPrologue(mod, before);
      break;
    case INTERRUPT_EPILOGUE:
      if (mod->getFunction("inception_interrupt_epilogue") != NULL) return;

      FNHInterruptEpilogue(mod, before);
      break;
    case ICP:
      FNHICP(mod, before);
      break;
    case INTERRUPT_HANDLER:
      FNHInterruptHandler(mod, before);
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

  if (inst == NULL) return;

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

  if (inst == NULL) return;

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

void FunctionsHelperWriter::FNHInterruptEpilogue(llvm::Module* mod,
                                                 llvm::Instruction* inst) {
  Constant* func_ptr = GetVoidFunctionPointer("inception_interrupt_epilogue");
  Function* function = cast<Function>(func_ptr);

  if (function == NULL) {
    inception_warning("Cannot write inception_interrupt_epilogue");
    return;
  }

  inception_message("Writing inception_interrupt_epilogue");

  if (function->empty()) {
    BasicBlock::Create(IContext::getContextRef(), "entry", function);
  }

  llvm::IRBuilder<>* IRB = new IRBuilder<>(&(function->getEntryBlock()));

  Value* reg = NULL;
  Value* sp = ReadReg(Reg("SP"), IRB, 32);

  llvm::StringRef targets[] = {"R0",  "R1", "R2", "R3",
                               "R12", "LR", "PC", "xPSR"};

  // Type* Ty_word = IntegerType::get(IContext::getContextRef(), 32);

  // AllocaInst* APSR = IRB->CreateAlloca(Ty_word);

  for (auto target : targets) {
    /*xPSR is build at run-time*/
    if (target.equals("xPSR")) {
      // APSR |= ( NF & 0x1 ) << 31;
      // APSR |= ( ZF & 0x1 ) << 30;
      // APSR |= ( CF & 0x1 ) << 29;
      // APSR |= ( VF & 0x1 ) << 28;
      // APSR |= ( QF & 0x1 ) << 27;

      Value* flags[5];
      flags[0] = Reg("NF");
      flags[1] = Reg("ZF");
      flags[2] = Reg("CF");
      flags[3] = Reg("VF");
      flags[4] = Reg("QF");

      // Retrieve APSR from stack
      reg = ReadReg(sp, IRB, 32);

      // Dispatch APSR into flags
      uint32_t shift = 31;
      for (auto flag : flags) {
        // 0x1 << shift;
        Value* val = IRB->CreateShl(getConstant(1), getConstant(shift));
        // AND APSR -> retrieve bit shift
        val = IRB->CreateAnd(reg, val);
        // Shift to the other direction
        val = IRB->CreateLShr(val, getConstant(shift--));

        WriteReg(val, flag, IRB, 32);
      }
      sp = UpdateRd(sp, getConstant("4"), IRB, true);
    } else {
      reg = ReadReg(sp, IRB, 32);
      WriteReg(reg, Reg(target), IRB, 32);
      sp = UpdateRd(sp, getConstant("4"), IRB, true);
    }
  }
  WriteReg(sp, Reg("SP"), IRB);

  IRB->CreateRetVoid();

  FunctionPassManager FPM(IContext::Mod);
  FPM.add(createNameRecoveryPass());
  FPM.run(*function);

  inception_message("Done");
}

void FunctionsHelperWriter::FNHInterruptPrologue(llvm::Module* mod,
                                                 llvm::Instruction* inst) {
  Constant* func_ptr = GetVoidFunctionPointer("inception_interrupt_prologue");
  Function* function = cast<Function>(func_ptr);
  if (function == NULL) {
    inception_warning("Cannot write inception_interrupt_prologue");
    return;
  }

  inception_message("Writing inception_interrupt_prologue");

  if (function->empty()) {
    BasicBlock::Create(IContext::getContextRef(), "entry", function);
  }

  llvm::IRBuilder<>* IRB = new IRBuilder<>(&(function->getEntryBlock()));

  Value* reg = NULL;
  Value* sp = ReadReg(Reg("SP"), IRB, 32);

  llvm::StringRef targets[] = {"xPSR", "PC", "LR", "R12",
                               "R3",   "R2", "R1", "R0"};

  Type* Ty_word = IntegerType::get(IContext::getContextRef(), 32);

  AllocaInst* APSR = IRB->CreateAlloca(Ty_word);

  for (auto target : targets) {
    /*xPSR is build at run-time*/
    if (target.equals("xPSR")) {
      // APSR |= ( NF & 0x1 ) << 31;
      // APSR |= ( ZF & 0x1 ) << 30;
      // APSR |= ( CF & 0x1 ) << 29;
      // APSR |= ( VF & 0x1 ) << 28;
      // APSR |= ( QF & 0x1 ) << 27;

      Value* flags[5];
      flags[0] = Reg("NF");
      flags[1] = Reg("ZF");
      flags[2] = Reg("CF");
      flags[3] = Reg("VF");
      flags[4] = Reg("QF");

      IRB->CreateStore(getConstant(0), APSR);

      uint32_t shift = 31;
      for (auto flag : flags) {
        Value* val = IRB->CreateLoad(flag);
        val = IRB->CreateAnd(val, getConstant(1));
        val = IRB->CreateShl(val, getConstant(shift--));
        Value* apsr = IRB->CreateLoad(APSR);
        val = IRB->CreateOr(apsr, val);
        IRB->CreateStore(val, APSR);
      }
      reg = IRB->CreateLoad(APSR);

    } else {
      reg = ReadReg(Reg(target), IRB, 32);
    }
    sp = UpdateRd(sp, getConstant("4"), IRB, false);
    WriteReg(reg, sp, IRB, 32);
  }

  WriteReg(sp, Reg("SP"), IRB, 32);
  WriteReg(getConstant("4"), Reg("LR"), IRB, 32);

  IRB->CreateRetVoid();

  inception_message("Done");

  FunctionPassManager FPM(IContext::Mod);
  FPM.add(createNameRecoveryPass());
  FPM.run(*function);
}

void FunctionsHelperWriter::FNHICP(llvm::Module* mod, llvm::Instruction* inst) {
  Constant* func_ptr = GetIntFunctionPointer("inception_icp");
  Function* function = cast<Function>(func_ptr);
  if (function == NULL) {
    inception_warning("Cannot write inception_icp");
    return;
  }

  inception_message("Writing inception_icp");

  if (function->empty()) {
    BasicBlock::Create(IContext::getContextRef(), "entry", function);
  }

  llvm::IRBuilder<>* IRB = new IRBuilder<>(&(function->getEntryBlock()));

  /*
   * We need to remove the last address bit which has no mean in Thumb/Thumb2
   * So, the adress 0x20000001 and 0x20000000 are the same
   */

  // 1) Retrieve function first argument
  Function::arg_iterator args = function->arg_begin();
  Value* param_1 = args++;
  param_1->setName("address");

  Type* Ty_word = IntegerType::get(IContext::getContextRef(), 32);

  AllocaInst* ptr_param1 = IRB->CreateAlloca(Ty_word);

  IRB->CreateStore(param_1, ptr_param1);

  // param1 contains the first argument of the function
  Value* param1 = IRB->CreateLoad(ptr_param1);
  // Address contains param1 without the first bit
  Value* address = IRB->CreateAnd(param1, getConstant("4294967294"));

  /*
   * We need to convert all the function symbols in an entry into the icp switch
   */
  object::ObjectFile* Executable =
      IContext::alm->Dec->getDisassembler()->getExecutable();

  uint64_t SymAddr;
  std::error_code ec;
  StringRef NameRef;

  IRBuilder<>* bbIRB = NULL;

  BasicBlock* entry_block =
      BasicBlock::Create(IContext::getContextRef(), "entry", function);

  IRB->CreateBr(entry_block);

  unsigned num_cases = 0;
  std::vector<ConstantInt*> addresses;
  std::vector<BasicBlock*> blocks;
  for (object::symbol_iterator I = Executable->symbols().begin(),
                               E = Executable->symbols().end();
       I != E; ++I) {
    object::SymbolRef::Type SymbolTy;
    if ((ec = I->getType(SymbolTy))) {
      errs() << ec.message() << "\n";
      continue;
    }
    if (SymbolTy != object::SymbolRef::ST_Function) {
      continue;
    }
    if ((ec = I->getAddress(SymAddr))) {
      errs() << ec.message() << "\n";
      continue;
    }
    if ((ec = I->getName(NameRef))) {
      errs() << ec.message() << "\n";
      continue;
    }

    if (NameRef.empty()) {
      std::string* FName = new std::string();
      raw_string_ostream FOut(*FName);
      FOut << "func_" << format("%1" PRIx64, (unsigned)SymAddr);
      NameRef = StringRef(FOut.str());
    }

    /*
     * For each function we need to create a basic block with a call inside.
     * This BB will be called by the switch case if the address match
     */
    BasicBlock* bb = BasicBlock::Create(IContext::getContextRef(),
                                        "call_" + NameRef.str(), function);

    /*
     * We now need to add the concrete call to this basic block
     */
    bbIRB = new IRBuilder<>(bb);
    CreateCall(NULL, bbIRB, (unsigned)SymAddr);
    bbIRB->CreateRetVoid();
    delete bbIRB;
    num_cases++;

    SymAddr &= 0xfffffffe;  // remove last bit
    ConstantInt* addr = ConstantInt::get(IContext::getContextRef(),
                                         APInt(32, (unsigned)SymAddr, 10));
    addresses.push_back(addr);
    blocks.push_back(bb);
  }

  BasicBlock* end_block =
      BasicBlock::Create(IContext::getContextRef(), "end", function);
  bbIRB = new IRBuilder<>(end_block);
  bbIRB->CreateRetVoid();
  delete bbIRB;

  IRBuilder<>* entryIRB = new IRBuilder<>(entry_block);
  SwitchInst* sw = entryIRB->CreateSwitch(address, end_block, num_cases);

  // add all cases
  // if more than one symbol has the same address, put only the first
  sw->addCase(addresses[0], blocks[0]);
  for (unsigned int i = 1; i < num_cases; i++) {
    bool duplicate = false;
    for (unsigned int j = 0; j < i; j++) {
      if (addresses[i] == addresses[j]) {
        duplicate = true;
        inception_warning(
            "[BranchHandlerBLXr] icp creation, duplicate function symbol at "
            "the same address, skipping it\n");
        break;
      }
    }
    if (!duplicate) {
      sw->addCase(addresses[i], blocks[i]);
    }
  }

  inception_message("Done");

  // FunctionPassManager FPM(IContext::Mod);
  // FPM.add(createNameRecoveryPass());
  // FPM.run(*function);
}

void FunctionsHelperWriter::FNHInterruptHandler(llvm::Module* mod,
                                                llvm::Instruction* inst) {
  Constant* func_ptr = GetIntFunctionPointer("inception_interrupt_handler");
  Function* function = cast<Function>(func_ptr);
  if (function == NULL) {
    inception_warning("Cannot write inception_interrupt_handler");
    return;
  }

  inception_message("Writing inception_interrupt_handler");

  if (function->empty()) {
    BasicBlock::Create(IContext::getContextRef(), "entry", function);
  }

  llvm::IRBuilder<>* IRB = new IRBuilder<>(&(function->getEntryBlock()));

  // 1) Retrieve function first argument
  Function::arg_iterator args = function->arg_begin();
  Value* param_1 = args++;
  param_1->setName("address");

  Type* Ty_word = IntegerType::get(IContext::getContextRef(), 32);

  AllocaInst* ptr_param1 = IRB->CreateAlloca(Ty_word);

  IRB->CreateStore(param_1, ptr_param1);
  // param1 contains the first argument of the function
  Value* param1 = IRB->CreateLoad(ptr_param1);
  // Address contains param1 without the first bit

  Constant* func_a = GetVoidFunctionPointer("inception_interrupt_prologue");
  Constant* func_b = GetIntFunctionPointer("inception_icp");
  Constant* func_c = GetVoidFunctionPointer("inception_interrupt_epilogue");

  IRB->CreateCall(func_a);
  IRB->CreateCall(func_b, param1);
  IRB->CreateCall(func_c);

  IRB->CreateRetVoid();

  inception_message("done");
}
