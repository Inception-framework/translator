#include "Target/ARM/BranchLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include <vector>

using namespace llvm;
using namespace fracture;

void BranchLifter::registerLifter() {
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tBX_RET,
                      (LifterHandler)&BranchLifter::BranchHandler);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tB,
                      (LifterHandler)&BranchLifter::BranchHandlerB);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tBcc,
                      (LifterHandler)&BranchLifter::BranchHandlerB);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tBL,
                      (LifterHandler)&BranchLifter::BranchHandlerBL);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::t2B,
                      (LifterHandler)&BranchLifter::BranchHandlerB);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::t2Bcc,
                      (LifterHandler)&BranchLifter::BranchHandlerB);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tBLXr,
                      (LifterHandler)&BranchLifter::BranchHandlerBLXr);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tCBZ,
                      (LifterHandler)&BranchLifter::BranchHandlerCB);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tCBNZ,
                      (LifterHandler)&BranchLifter::BranchHandlerCB);
}

void BranchLifter::BranchHandler(SDNode *N, IRBuilder<> *IRB) {
  Instruction *Ret = IRB->CreateRetVoid();
  Ret->setDebugLoc(N->getDebugLoc());
  alm->VisitMap[N] = Ret;
  return;
}

void BranchLifter::BranchHandlerB(SDNode *N, IRBuilder<> *IRB) {
  // Get the address
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(1));
  if (!DestNode) {
    outs() << "visitBRCOND: Not a constant integer for branch!\n";
    return;
  }

  uint32_t DestInt = DestNode->getSExtValue();
  uint32_t PC = alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  // Note: pipeline is 8 bytes
  outs() << "Target = PC + 4 + DestInt = " << PC << " + 4 + " << DestInt
         << "\n";
  uint32_t Tgt = PC + 4 + DestInt;

  Function *F = IRB->GetInsertBlock()->getParent();
  BasicBlock *CurBB = IRB->GetInsertBlock();

  BasicBlock *BBTgt = alm->Dec->getOrCreateBasicBlock(Tgt, F);

  // Parse the branch condition code
  const ConstantSDNode *CCNode = dyn_cast<ConstantSDNode>(N->getOperand(2));
  if (!CCNode) {
    errs() << "visitBRCOND: Condition code is not a constant integer!\n";
    return;
  }
  ARMCC::CondCodes ARMcc = ARMCC::CondCodes(CCNode->getZExtValue());

  // Unconditional branch
  if (ARMcc == ARMCC::AL) {
    Instruction *Br = IRB->CreateBr(BBTgt);
    Br->setDebugLoc(N->getDebugLoc());
    alm->VisitMap[N] = Br;
    return;
  }

  // If conditional branch, find the successor block and look at CC
  BasicBlock *NextBB = NULL;
  Function::iterator BI = F->begin(), BE = F->end();
  while (BI != BE && BI->getName() != CurBB->getName()) ++BI;
  ++BI;
  if (BI == BE) {  // NOTE: This should never happen...
    NextBB = alm->Dec->getOrCreateBasicBlock("end", F);
  } else {
    NextBB = &(*BI);
  }

  // SDNode *CPSR = N->getOperand(3)->getOperand(1).getNode();
  // CPSR->dump();
  // SDNode *CMPNode = NULL;
  // for (SDNode::use_iterator I = CPSR->use_begin(), E = CPSR->use_end(); I !=
  // E;
  //     ++I) {
  //  if (I->getOpcode() == ISD::CopyToReg) {
  //    CMPNode = I->getOperand(2).getNode();
  //  }
  //}

  //// TODO: maybe we could just always use flags?
  // if (CMPNode == NULL) {
  //  errs()
  //      << "ARMIREmitter ERROR: Could not find CMP SDNode for ARMBRCond !\n ";
  //  return;
  //}

  // CMPNode->dump();

  Value *Cmp = NULL;
  Value *Cmp1 = NULL;
  Value *Cmp2 = NULL;
  // Value *LHS = visit(CMPNode->getOperand(0).getNode(), IRB);
  // Value *RHS = visit(CMPNode->getOperand(1).getNode(), IRB);
  // See ARMCC::CondCodes IntCCToARMCC(ISD::CondCode CC); in ARMISelLowering.cpp
  // TODO: Add support for conditions that handle floating point
  switch (ARMcc) {
    default:
      errs() << "Unknown condition code\n";
      return;
    case ARMCC::EQ:
      // Cmp = IRB->CreateICmpEQ(LHS, RHS);
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("1"));
      break;
    case ARMCC::NE:
      // Cmp = IRB->CreateICmpNE(LHS, RHS);
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("0"));
      break;
    case ARMCC::HS:
      // HS - unsigned higher or same (or carry set)
      // Cmp = IRB->CreateICmpUGE(LHS, RHS);
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("CF"), IRB), getConstant("1"));
      break;
    case ARMCC::LO:
      // LO - unsigned lower (or carry clear)
      // Cmp = IRB->CreateICmpULT(LHS, RHS);
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("CF"), IRB), getConstant("0"));
      break;
    case ARMCC::MI:
      // MI - minus (negative)
      // errs() << "Condition code MI is not handled at this time!\n";
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("NF"), IRB), getConstant("1"));
      break;
    // break;
    case ARMCC::PL:
      // PL - plus (positive or zero)
      // errs() << "Condition code PL is not handled at this time!\n";
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("NF"), IRB), getConstant("0"));
      break;
    // break;
    case ARMCC::VS:
      // VS - V Set (signed overflow)
      // errs() << "Condition code VS is not handled at this time!\n";
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("VF"), IRB), getConstant("1"));
      break;
    // break;
    case ARMCC::VC:
      // VC - V clear (no signed overflow)
      // errs() << "Condition code VC is not handled at this time!\n";
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("VF"), IRB), getConstant("0"));
      break;
    // break;
    case ARMCC::HI:
      // HI - unsigned higher
      // Cmp = IRB->CreateICmpUGT(LHS, RHS);
      Cmp1 = IRB->CreateICmpEQ(ReadReg(Reg("CF"), IRB), getConstant("1"));
      Cmp2 = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("0"));
      Cmp = IRB->CreateAnd(Cmp1, Cmp2);
      break;
    case ARMCC::LS:
      // LS - unsigned lower or same
      // Cmp = IRB->CreateICmpULE(LHS, RHS);
      Cmp1 = IRB->CreateICmpEQ(ReadReg(Reg("CF"), IRB), getConstant("0"));
      Cmp2 = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("1"));
      Cmp = IRB->CreateOr(Cmp1, Cmp2);
      break;
    case ARMCC::GE:
      // GE - signed greater or equal
      // Cmp = IRB->CreateICmpSGE(LHS, RHS);
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("NF"), IRB), ReadReg(Reg("VF"), IRB));
      break;
    case ARMCC::LT:
      // LT - signed less than
      // Cmp = IRB->CreateICmpSLT(LHS, RHS);
      Cmp = IRB->CreateICmpNE(ReadReg(Reg("NF"), IRB), ReadReg(Reg("VF"), IRB));
      break;
    case ARMCC::GT:
      // GT - signed greater than
      // Cmp = IRB->CreateICmpSGT(LHS, RHS);
      Cmp1 = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("0"));
      Cmp2 =
          IRB->CreateICmpEQ(ReadReg(Reg("NF"), IRB), ReadReg(Reg("VF"), IRB));
      Cmp = IRB->CreateAnd(Cmp1, Cmp2);
      break;
    case ARMCC::LE:
      // LE - signed less than or equal
      // Cmp = IRB->CreateICmpSLE(LHS, RHS);
      Cmp1 = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("1"));
      Cmp2 =
          IRB->CreateICmpNE(ReadReg(Reg("NF"), IRB), ReadReg(Reg("VF"), IRB));
      Cmp = IRB->CreateOr(Cmp1, Cmp2);
      break;
  }
  (dyn_cast<Instruction>(Cmp))->setDebugLoc(N->getOperand(3)->getDebugLoc());

  // Conditional branch
  Instruction *Br = IRB->CreateCondBr(Cmp, BBTgt, NextBB);
  Br->setDebugLoc(N->getDebugLoc());
  alm->VisitMap[N] = Br;
  return;
}

// TODO: handle conditions in factorized code for all branch types
// This version of BL does not handle conditions
void BranchLifter::BranchHandlerBL(SDNode *N, IRBuilder<> *IRB) {
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(3));
  if (!DestNode) {
    outs() << "visitCALL: Not a constant integer for call!";
    return;
  }

  int64_t DestInt = DestNode->getSExtValue();
  int64_t PC = alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  int64_t Tgt = PC + 4 + DestInt;
  CallHandler(N, IRB, Tgt);
}

// simple indirect call promotion
void BranchLifter::BranchHandlerBLXr(SDNode *N, IRBuilder<> *IRB) {
  // pointer register
  Value *Rm = visit(N->getOperand(3).getNode(), IRB);
  Rm = IRB->CreateAnd(Rm, getConstant("4294967294"));  // remove last bit

  // if necessary create the indirect call promotion function
  Function *icp = alm->Mod->getFunction("icp");
  if (icp == NULL) {
    Constant *c = alm->Mod->getOrInsertFunction(
        "icp", Type::getVoidTy(alm->getContextRef()),
        IntegerType::get(alm->getContextRef(), 32), NULL);
    icp = cast<Function>(c);
    icp->setCallingConv(CallingConv::C);
    Function::arg_iterator args = icp->arg_begin();
    Value *ptr_reg = args;
    ptr_reg->setName("ptr_reg");

    // for each function (address/name) in the symbol table
    object::ObjectFile *Executable =
        alm->Dec->getDisassembler()->getExecutable();
    uint64_t SymAddr;
    std::error_code ec;
    StringRef NameRef;

    IRBuilder<> *bbIRB = NULL;

    BasicBlock *entry_block =
        BasicBlock::Create(alm->getContextRef(), "entry", icp);

    unsigned num_cases = 0;
    std::vector<ConstantInt *> addresses;
    std::vector<BasicBlock *> blocks;
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
        std::string *FName = new std::string();
        raw_string_ostream FOut(*FName);
        FOut << "func_" << format("%1" PRIx64, (unsigned)SymAddr);
        NameRef = StringRef(FOut.str());
      }
      // if the register points to the function, call it
      outs() << "FUNCTION " << NameRef << " " << (unsigned)SymAddr << "\n";
      BasicBlock *bb = BasicBlock::Create(alm->getContextRef(),
                                          "call_" + NameRef.str(), icp);
      bbIRB = new IRBuilder<>(bb);
      CallHandler(NULL, bbIRB, (unsigned)SymAddr);
      bbIRB->CreateRetVoid();
      delete bbIRB;
      num_cases++;
      // Value *cmp = prevIRB->CreateICmpEQ(
      //    ptr_reg, ConstantInt::get(alm->getContextRef(),
      //                              APInt(32, (unsigned)SymAddr, 10)));
      SymAddr &= 0xfffffffe;  // remove last bit
      ConstantInt *addr = ConstantInt::get(alm->getContextRef(),
                                           APInt(32, (unsigned)SymAddr, 10));
      addresses.push_back(addr);
      blocks.push_back(bb);
    }

    BasicBlock *end_block =
        BasicBlock::Create(alm->getContextRef(), "end", icp);
    bbIRB = new IRBuilder<>(end_block);
    bbIRB->CreateRetVoid();
    delete bbIRB;

    IRBuilder<> *entryIRB = new IRBuilder<>(entry_block);
    SwitchInst *sw = entryIRB->CreateSwitch(ptr_reg, end_block, num_cases);

    for (int i = 0; i < num_cases; i++) {
      sw->addCase(addresses[i], blocks[i]);
    }
  }

  IRB->CreateCall(icp, Rm);
  Value *dummyLR = getConstant("0");
  alm->VisitMap[N] = dummyLR;
}

void BranchLifter::CallHandler(SDNode *N, IRBuilder<> *IRB, uint32_t Tgt) {
  // TODO: Look up address in symbol table.
  std::string FName = alm->Dec->getDisassembler()->getFunctionName(Tgt);

  Module *Mod = IRB->GetInsertBlock()->getParent()->getParent();

  Function *Func = Mod->getFunction(FName);
  if (Func == NULL) {
    outs() << "[CallHanlder] Error, Function to call is NULL\n";
    exit(0);
  }

  // TODO: type cast!
  // outs() << FName << " args:\n";
  std::vector<Value *> Args;
  std::vector<Type *> ArgTypes;
  char reg_name[3] = "R0";
  for (Function::arg_iterator I = Func->arg_begin(), E = Func->arg_end();
       I != E; ++I) {
    ArgTypes.push_back(I->getType());
    if (I->getType()->isPointerTy()) {
      Value *Res = ReadReg(Reg(reg_name), IRB);
      // Res = IRB->CreatePtrToInt(Res, IntegerType::get(alm->getContext(),
      // 32));
      Res = IRB->CreateIntToPtr(Res, I->getType());
      Args.push_back(Res);
    } else {
      Args.push_back(ReadReg(Reg(reg_name), IRB));
    }
    // Type *arg_ty = I->getType();
    // if (I->getType() != IntegerType::get(alm->getContextRef(), 32)) {
    //   outs() << "Warning: unsupported parameter type for function call "
    //          << FName << "\n";
    //   if (N != NULL) {
    //     Value *dummyLR = getConstant("0");
    //     alm->VisitMap[N] = dummyLR;
    //   }
    //   return;
    // }
    // // arg_ty->dump();
    // Value *arg_reg = ReadReg(Reg(reg_name), IRB);
    // ArgTypes.push_back(arg_ty);
    // Args.push_back(arg_reg);
    reg_name[1]++;
  }

  // if (Func->getReturnType() != IntegerType::get(alm->getContextRef(), 32)) {
  //  outs() << "Warning: unsupported return type for function call " << FName
  //         << "\n";
  //  if (N != NULL) {
  //    Value *dummyLR = getConstant("0");
  //    alm->VisitMap[N] = dummyLR;
  //  }
  //  return;
  //}

  FunctionType *FT = FunctionType::get(
      // Type::getPrimitiveType(alm->getContext(), Type::VoidTyID), false);
      Func->getReturnType(), ArgTypes, false);

  Twine TgtAddr(Tgt);

  outs() << " =========================== \n\n";
  outs() << "Tgt        :  " << format("%8" PRIx64, Tgt) << '\n';
  outs() << "instrSize  :  " << format("%8" PRIx64, 4) << '\n';
  // outs() << "DestInt    :  " << format("%8" PRIx64, DestInt) << '\n';
  // outs() << "PC         :  " << format("%8" PRIx64, PC) << '\n';
  outs() << "FName      :  " << FName << '\n';
  outs() << " =========================== \n\n";

  AttributeSet AS;
  AS = AS.addAttribute(alm->getContextRef(), AttributeSet::FunctionIndex,
                       "Address", TgtAddr.str());

  Function *Proto = cast<Function>(Mod->getOrInsertFunction(FName, FT, AS));
  // Value *Proto = Mod->getOrInsertFunction(
  //    FName, FunctionType::get(Func->getReturnType(), ArgTypes, false), AS);

  Proto->setCallingConv(Func->getCallingConv());
  Value *Call = IRB->CreateCall(dyn_cast<Value>(Proto), Args);
  if (!Func->getReturnType()->isVoidTy()) WriteReg(Call, Reg("R0"), IRB);

  if (N != NULL) {
    Value *dummyLR = getConstant("0");
    alm->VisitMap[N] = dummyLR;
  }
  return;

  // TODO: Technically visitCall sets the LR to IP+8. We should return that.
}

void BranchLifter::BranchHandlerCB(SDNode *N, IRBuilder<> *IRB) {
  // Get the address
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(2));
  if (!DestNode) {
    outs() << "visitBRCOND: Not a constant integer for branch!\n";
    return;
  }

  uint32_t DestInt = DestNode->getSExtValue();
  uint32_t PC = alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  // Note: pipeline is 8 bytes
  outs() << "Target = PC + 4 + DestInt = " << PC << " + 4 + " << DestInt
         << "\n";
  uint32_t Tgt = PC + 4 + DestInt;

  Function *F = IRB->GetInsertBlock()->getParent();
  BasicBlock *CurBB = IRB->GetInsertBlock();

  BasicBlock *BBTgt = alm->Dec->getOrCreateBasicBlock(Tgt, F);

  // get the condition register
  Value *Rn = visit(N->getOperand(1).getNode(), IRB);

  // find the successor block
  BasicBlock *NextBB = NULL;
  Function::iterator BI = F->begin(), BE = F->end();
  while (BI != BE && BI->getName() != CurBB->getName()) ++BI;
  ++BI;
  if (BI == BE) {  // NOTE: This should never happen...
    NextBB = alm->Dec->getOrCreateBasicBlock("end", F);
  } else {
    NextBB = &(*BI);
  }

  // Compute the condition
  Value *Cmp = NULL;
  switch (N->getMachineOpcode()) {
    default:
      errs() << "[BranchHandlerCB] Error, unknown opcode\n";
      return;
    case ARM::tCBZ:
      Cmp = IRB->CreateICmpEQ(Rn, getConstant("0"));
      break;
    case ARM::tCBNZ:
      Cmp = IRB->CreateICmpNE(Rn, getConstant("0"));
      break;
  }

  // Conditional branch
  Instruction *Br = IRB->CreateCondBr(Cmp, BBTgt, NextBB);
  Br->setDebugLoc(N->getDebugLoc());
  alm->VisitMap[N] = Br;
  return;
}

