#include "Target/ARM/BranchLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

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

  // TODO: Look up address in symbol table.
  std::string FName = alm->Dec->getDisassembler()->getFunctionName(Tgt);

  Module *Mod = IRB->GetInsertBlock()->getParent()->getParent();

  Function *Func = Mod->getFunction(FName);
  if (Func == NULL) {
    outs() << "Error, Func == NULL\n";
    exit(0);
  }

  std::vector<Value *> Args;
  std::vector<Type *> ArgTypes;
  char reg_name[3] = "R0";
  for (Function::arg_iterator I = Func->arg_begin(), E = Func->arg_end();
       I != E; ++I) {
    ArgTypes.push_back(I->getType());
    Args.push_back(ReadReg(Reg(reg_name), IRB));
    reg_name[1]++;
  }

  FunctionType *FT = FunctionType::get(
      // Type::getPrimitiveType(Mod->getContext(), Type::VoidTyID), false);
      Func->getReturnType(), ArgTypes, false);

  Twine TgtAddr(Tgt);

  outs() << " =========================== \n\n";
  outs() << "Tgt        :  " << format("%8" PRIx64, Tgt) << '\n';
  outs() << "instrSize  :  " << format("%8" PRIx64, 4) << '\n';
  outs() << "DestInt    :  " << format("%8" PRIx64, DestInt) << '\n';
  outs() << "PC         :  " << format("%8" PRIx64, PC) << '\n';
  outs() << "FName      :  " << FName << '\n';
  outs() << " =========================== \n\n";

  AttributeSet AS;
  AS = AS.addAttribute(Mod->getContext(), AttributeSet::FunctionIndex,
                       "Address", TgtAddr.str());

  Function *Proto = cast<Function>(Mod->getOrInsertFunction(FName, FT, AS));
  // Value *Proto = Mod->getOrInsertFunction(
  //    FName, FunctionType::get(Func->getReturnType(), ArgTypes, false), AS);

  Proto->setCallingConv(Func->getCallingConv());
  Value *Call = IRB->CreateCall(dyn_cast<Value>(Proto), Args);
  if (!Func->getReturnType()->isVoidTy()) WriteReg(Call, Reg("R0"), IRB);

  Value *dummyLR = getConstant("0");
  alm->VisitMap[N] = dummyLR;
  return;

  // TODO: Technically visitCall sets the LR to IP+8. We should return that.
}
