
/*
    This file is part of Inception translator.

    Inception translator is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Inception translator.  If not, see <https://www.gnu.org/licenses/>.

    Copyright (c) 2017 Maxim Integrated, Inc.
    Author: Nassim Corteggiani <nassim.corteggiani@maximintegrated.com>

    Copyright (c) 2017 EURECOM, Inc.
    Author: Giovanni Camurati <giovanni.camurati@eurecom.fr>
*/

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
    case SWITCH_SP:
      FNHSwitchSP(mod, before);
      break;
    case WRITEBACK_SP:
      FNHWritebackSP(mod, before);
      break;
    case CACHE_SP:
      FNHCacheSP(mod, before);
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

  // switch sp based on EXC_RETURN
  // this value is written into PC by bx lr, pop {pc} etc.
  Value* EXC_RETURN = ReadReg(Reg("PC"), IRB);
  EXC_RETURN = IRB->CreateLShr(EXC_RETURN, getConstant(2));
  EXC_RETURN = IRB->CreateAnd(EXC_RETURN, getConstant(1));
  Constant* switch_sp = GetVoidFunctionPointer("inception_switch_sp");
  IRB->CreateCall(switch_sp, EXC_RETURN);

  // unstacking
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

  // stacking
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
  WriteReg(sp, Reg("SP"), IRB);

  // Write EXC_RETURN in LR
  Value* EXC_RETURN = getConstant(0xfffffff0);    // base value
  EXC_RETURN =
      IRB->CreateOr(EXC_RETURN, getConstant(0x8));  // always return in thread
  // mode, nested exceptions are
  // not supported for now
  EXC_RETURN = IRB->CreateOr(EXC_RETURN,
                             IRB->CreateShl(ReadReg(Reg("CONTROL_1"), IRB),
                                            getConstant(2)));  // PSP/MSP mode
  EXC_RETURN = IRB->CreateOr(EXC_RETURN, getConstant(0x1));    // Thumb/ARM
  WriteReg(EXC_RETURN, Reg("LR"), IRB);

  // switch sp to MSP
  Constant* switch_sp = GetVoidFunctionPointer("inception_switch_sp");
  IRB->CreateCall(switch_sp, getConstant("0"));

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

  // In case we reach the default block, it means we are trying to call an
  // address which does not correpond to the entry point of a function in the
  // symbols table. We do a sort of weak CFI and report an error message
  // Type Definitions

  // error info
  ArrayType* ArrayTy_0 =
      ArrayType::get(IntegerType::get(mod->getContext(), 8), 24);

  ArrayType* ArrayTy_2 =
      ArrayType::get(IntegerType::get(mod->getContext(), 8), 98);

  ArrayType* ArrayTy_4 =
      ArrayType::get(IntegerType::get(mod->getContext(), 8), 1);

  GlobalVariable* gvar_array_inception_icp_error_message_filename =
      new GlobalVariable(/*Module=*/*mod,
                         /*Type=*/ArrayTy_0,
                         /*isConstant=*/false,
                         /*Linkage=*/GlobalValue::ExternalLinkage,
                         /*Initializer=*/0,  // has initializer, specified below
                         /*Name=*/"inception_icp_error_message_filename");
  gvar_array_inception_icp_error_message_filename->setAlignment(1);

  GlobalVariable* gvar_array_inception_icp_error_message_message =
      new GlobalVariable(/*Module=*/*mod,
                         /*Type=*/ArrayTy_2,
                         /*isConstant=*/false,
                         /*Linkage=*/GlobalValue::ExternalLinkage,
                         /*Initializer=*/0,  // has initializer, specified below
                         /*Name=*/"inception_icp_error_message_message");
  gvar_array_inception_icp_error_message_message->setAlignment(1);

  GlobalVariable* gvar_int32_inception_icp_error_line =
      new GlobalVariable(/*Module=*/*mod,
                         /*Type=*/IntegerType::get(mod->getContext(), 32),
                         /*isConstant=*/false,
                         /*Linkage=*/GlobalValue::ExternalLinkage,
                         /*Initializer=*/0,  // has initializer, specified below
                         /*Name=*/"inception_icp_error_line");
  gvar_int32_inception_icp_error_line->setAlignment(4);

  GlobalVariable* gvar_array_inception_icp_error_message_suffix =
      new GlobalVariable(/*Module=*/*mod,
                         /*Type=*/ArrayTy_4,
                         /*isConstant=*/false,
                         /*Linkage=*/GlobalValue::ExternalLinkage,
                         /*Initializer=*/0,  // has initializer, specified below
                         /*Name=*/"inception_icp_error_message_suffix");
  gvar_array_inception_icp_error_message_suffix->setAlignment(1);

  // Constant Definitions
  Constant* const_array_5 = ConstantDataArray::getString(
      mod->getContext(), "Indirect Call Promotion", true);
  Constant* const_array_6 = ConstantDataArray::getString(
      mod->getContext(),
      "ICP encountered an address that is not an entry point of a function "
      "defined in the symbols table\x0A",
      true);
  ConstantInt* const_int32_7 =
      ConstantInt::get(mod->getContext(), APInt(32, StringRef("100"), 10));
  Constant* const_array_8 =
      ConstantDataArray::getString(mod->getContext(), "", true);

  // Global Variable Definitions
  gvar_array_inception_icp_error_message_filename->setInitializer(
      const_array_5);
  gvar_array_inception_icp_error_message_message->setInitializer(const_array_6);
  gvar_int32_inception_icp_error_line->setInitializer(const_int32_7);
  gvar_array_inception_icp_error_message_suffix->setInitializer(const_array_8);

  // call the error function
  BasicBlock* end_block =
      BasicBlock::Create(IContext::getContextRef(), "end", function);
  bbIRB = new IRBuilder<>(end_block);
  // Constant* const_ptr = GetVoidFunctionPointer("inception_report_error");
  Constant* const_ptr = GetVoidFunctionPointer("inception_warning");
  std::vector<Value*> Args;
  // Args.push_back(gvar_array_inception_icp_error_message_filename);
  // Args.push_back(gvar_int32_inception_icp_error_line);
  Args.push_back(gvar_array_inception_icp_error_message_message);
  // Args.push_back(gvar_array_inception_icp_error_message_suffix);
  Instruction* warning = bbIRB->CreateCall(const_ptr, Args);
  bbIRB->CreateRetVoid();
  delete bbIRB;

  IRBuilder<>* entryIRB = new IRBuilder<>(entry_block);
  SwitchInst* sw = entryIRB->CreateSwitch(address, end_block, num_cases);

  // add all cases
  // if more than one symbol has the same address, put only the first
  if( addresses.size() > 0 ) {
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

  Constant* prologue = GetVoidFunctionPointer("inception_interrupt_prologue");
  Constant* handler = GetIntFunctionPointer("inception_icp");
  Constant* epilogue = GetVoidFunctionPointer("inception_interrupt_epilogue");

  IRB->CreateCall(prologue);
  IRB->CreateCall(handler, param1);
  IRB->CreateCall(epilogue);

  IRB->CreateRetVoid();

  inception_message("done");
}

void FunctionsHelperWriter::FNHSwitchSP(llvm::Module* mod,
                                        llvm::Instruction* inst) {
  Constant* func_ptr = GetIntFunctionPointer("inception_switch_sp");
  Function* function = cast<Function>(func_ptr);
  if (function == NULL) {
    inception_warning("Cannot write inception_switch_sp");
    return;
  }

  inception_message("Writing inception_switch_sp");

  if (function->empty()) {
    BasicBlock::Create(IContext::getContextRef(), "wb", function);
  }

  llvm::IRBuilder<>* IRB = new IRBuilder<>(&(function->getEntryBlock()));

  // 1) Retrieve function first argument
  Function::arg_iterator args = function->arg_begin();
  Value* param_1 = args++;
  param_1->setName("CONTROL_1_TARGET");

  Type* Ty_word = IntegerType::get(IContext::getContextRef(), 32);

  AllocaInst* ptr_param1 = IRB->CreateAlloca(Ty_word);

  IRB->CreateStore(param_1, ptr_param1);
  // param1 contains the first argument of the function
  Value* ctrl1_tgt = IRB->CreateLoad(ptr_param1);

  // Basic blocks and builders
  BasicBlock* bb_wbToMSP =
      BasicBlock::Create(IContext::getContextRef(), "wbToMSP", function);
  BasicBlock* bb_wbToPSP =
      BasicBlock::Create(IContext::getContextRef(), "wbToPSP", function);
  BasicBlock* bb_cache =
      BasicBlock::Create(IContext::getContextRef(), "cache", function);
  BasicBlock* bb_cacheMSP =
      BasicBlock::Create(IContext::getContextRef(), "cacheMSP", function);
  BasicBlock* bb_cachePSP =
      BasicBlock::Create(IContext::getContextRef(), "cachePSP", function);
  BasicBlock* bb_end =
      BasicBlock::Create(IContext::getContextRef(), "end", function);

  IRBuilder<>* irb_wbToMSP = new IRBuilder<>(bb_wbToMSP);
  IRBuilder<>* irb_wbToPSP = new IRBuilder<>(bb_wbToPSP);
  IRBuilder<>* irb_cache = new IRBuilder<>(bb_cache);
  IRBuilder<>* irb_cacheMSP = new IRBuilder<>(bb_cacheMSP);
  IRBuilder<>* irb_cachePSP = new IRBuilder<>(bb_cachePSP);
  IRBuilder<>* irb_end = new IRBuilder<>(bb_end);

  // Update MSP or PSP based on CONTROL_1 (i.e. write back SP because it could
  // be dirty)
  SwitchInst* wb_sw =
      IRB->CreateSwitch(ReadReg(Reg("CONTROL_1"), IRB), bb_cache, 2);
  ConstantInt* zero =
      ConstantInt::get(IContext::getContextRef(), APInt(32, 0, 10));
  ConstantInt* one =
      ConstantInt::get(IContext::getContextRef(), APInt(32, 1, 10));
  wb_sw->addCase(zero, bb_wbToMSP);
  wb_sw->addCase(one, bb_wbToPSP);

  WriteReg(ReadReg(Reg("SP"), irb_wbToMSP), Reg("MSP"), irb_wbToMSP);
  WriteReg(ReadReg(Reg("SP"), irb_wbToPSP), Reg("PSP"), irb_wbToPSP);

  irb_wbToMSP->CreateBr(bb_cache);
  irb_wbToPSP->CreateBr(bb_cache);

  // Update SP and CONTROL_1 based on CONTROL_1_TARGET (i.e. cache the desired
  // stack pointer in SP)
  SwitchInst* cache_sw = irb_cache->CreateSwitch(ctrl1_tgt, bb_end, 2);
  cache_sw->addCase(zero, bb_cacheMSP);
  cache_sw->addCase(one, bb_cachePSP);

  WriteReg(ReadReg(Reg("MSP"), irb_cacheMSP), Reg("SP"), irb_cacheMSP);
  WriteReg(ReadReg(Reg("PSP"), irb_cachePSP), Reg("SP"), irb_cachePSP);

  irb_cacheMSP->CreateBr(bb_end);
  irb_cachePSP->CreateBr(bb_end);

  WriteReg(ctrl1_tgt, Reg("CONTROL_1"), irb_end);

  irb_end->CreateRetVoid();

  inception_message("done");
}

void FunctionsHelperWriter::FNHWritebackSP(llvm::Module* mod,
                                           llvm::Instruction* inst) {
  Constant* func_ptr = GetVoidFunctionPointer("inception_writeback_sp");
  Function* function = cast<Function>(func_ptr);
  if (function == NULL) {
    inception_warning("Cannot write inception_writeback_sp");
    return;
  }

  inception_message("Writing inception_writeback_sp");

  if (function->empty()) {
    BasicBlock::Create(IContext::getContextRef(), "wb", function);
  }

  llvm::IRBuilder<>* IRB = new IRBuilder<>(&(function->getEntryBlock()));

  // Basic blocks and builders
  BasicBlock* bb_wbToMSP =
      BasicBlock::Create(IContext::getContextRef(), "wbToMSP", function);
  BasicBlock* bb_wbToPSP =
      BasicBlock::Create(IContext::getContextRef(), "wbToPSP", function);
  BasicBlock* bb_end =
      BasicBlock::Create(IContext::getContextRef(), "end", function);

  IRBuilder<>* irb_wbToMSP = new IRBuilder<>(bb_wbToMSP);
  IRBuilder<>* irb_wbToPSP = new IRBuilder<>(bb_wbToPSP);
  IRBuilder<>* irb_end = new IRBuilder<>(bb_end);

  // Update MSP or PSP based on CONTROL_1 (i.e. write back SP because it could
  // be dirty)
  SwitchInst* wb_sw =
      IRB->CreateSwitch(ReadReg(Reg("CONTROL_1"), IRB), bb_end, 2);
  ConstantInt* zero =
      ConstantInt::get(IContext::getContextRef(), APInt(32, 0, 10));
  ConstantInt* one =
      ConstantInt::get(IContext::getContextRef(), APInt(32, 1, 10));
  wb_sw->addCase(zero, bb_wbToMSP);
  wb_sw->addCase(one, bb_wbToPSP);

  WriteReg(ReadReg(Reg("SP"), irb_wbToMSP), Reg("MSP"), irb_wbToMSP);
  WriteReg(ReadReg(Reg("SP"), irb_wbToPSP), Reg("PSP"), irb_wbToPSP);

  irb_wbToMSP->CreateBr(bb_end);
  irb_wbToPSP->CreateBr(bb_end);

  irb_end->CreateRetVoid();

  inception_message("done");
}

void FunctionsHelperWriter::FNHCacheSP(llvm::Module* mod,
                                       llvm::Instruction* inst) {
  Constant* func_ptr = GetVoidFunctionPointer("inception_cache_sp");
  Function* function = cast<Function>(func_ptr);
  if (function == NULL) {
    inception_warning("Cannot write inception_cache_sp");
    return;
  }

  inception_message("Writing inception_cache_sp");

  if (function->empty()) {
    BasicBlock::Create(IContext::getContextRef(), "wb", function);
  }

  llvm::IRBuilder<>* IRB = new IRBuilder<>(&(function->getEntryBlock()));

  // Basic blocks and builders
  BasicBlock* bb_cacheMSP =
      BasicBlock::Create(IContext::getContextRef(), "cacheMSP", function);
  BasicBlock* bb_cachePSP =
      BasicBlock::Create(IContext::getContextRef(), "cachePSP", function);
  BasicBlock* bb_end =
      BasicBlock::Create(IContext::getContextRef(), "end", function);

  IRBuilder<>* irb_cacheMSP = new IRBuilder<>(bb_cacheMSP);
  IRBuilder<>* irb_cachePSP = new IRBuilder<>(bb_cachePSP);
  IRBuilder<>* irb_end = new IRBuilder<>(bb_end);

  // Update SP based on CONTROL_1 (i.e. update the desired
  // stack pointer in SP)
  SwitchInst* cache_sw =
      IRB->CreateSwitch(ReadReg(Reg("CONTROL_1"), IRB), bb_end, 2);
  ConstantInt* zero =
      ConstantInt::get(IContext::getContextRef(), APInt(32, 0, 10));
  ConstantInt* one =
      ConstantInt::get(IContext::getContextRef(), APInt(32, 1, 10));
  cache_sw->addCase(zero, bb_cacheMSP);
  cache_sw->addCase(one, bb_cachePSP);

  WriteReg(ReadReg(Reg("MSP"), irb_cacheMSP), Reg("SP"), irb_cacheMSP);
  WriteReg(ReadReg(Reg("PSP"), irb_cachePSP), Reg("SP"), irb_cachePSP);

  irb_cacheMSP->CreateBr(bb_end);
  irb_cachePSP->CreateBr(bb_end);

  irb_end->CreateRetVoid();

  inception_message("done");
}

