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
}

void BranchLifter::BranchHandler(SDNode *N, IRBuilder<> *IRB) {
  IRB->CreateRetVoid();
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

  //  // Parse the branch condition code
  //  const ConstantSDNode *CCNode = dyn_cast<ConstantSDNode>(N->getOperand(1));
  //  if (!CCNode) {
  //    // printError("visitBRCOND: Condition code is not a constant integer!");
  //    return;
  //  }
  //  ARMCC::CondCodes ARMcc = ARMCC::CondCodes(CCNode->getZExtValue());
  //
  // Unconditional branch
  // if (ARMcc == ARMCC::AL) {
  Instruction *Br = IRB->CreateBr(BBTgt);
  Br->setDebugLoc(N->getDebugLoc());
  alm->VisitMap[N] = Br;
  // return Br;
  //}
  //
  //  // If not a conditional branch, find the successor block and look at CC
  //  BasicBlock *NextBB = NULL;
  //  Function::iterator BI = F->begin(), BE = F->end();
  //  while (BI != BE && BI->getName() != CurBB->getName()) ++BI;
  //  ++BI;
  //  if (BI == BE) {  // NOTE: This should never happen...
  //    NextBB = Dec->getOrCreateBasicBlock("end", F);
  //  } else {
  //    NextBB = &(*BI);
  //  }
  //
  //  SDNode *CPSR = N->getOperand(2)->getOperand(1).getNode();
  //  SDNode *CMPNode = NULL;
  //  for (SDNode::use_iterator I = CPSR->use_begin(), E = CPSR->use_end(); I !=
  //  E;
  //       ++I) {
  //    if (I->getOpcode() == ISD::CopyToReg) {
  //      CMPNode = I->getOperand(2).getNode();
  //    }
  //  }
  //
  //  if (CMPNode == NULL) {
  //    errs() << "ARMIREmitter ERROR: Could not find CMP SDNode for
  //    ARMBRCond!\n";
  //    return;
  //  }
  //
  //  Value *Cmp = NULL;
  //  Value *LHS = visit(CMPNode->getOperand(0).getNode());
  //  Value *RHS = visit(CMPNode->getOperand(1).getNode());
  //  // See ARMCC::CondCodes IntCCToARMCC(ISD::CondCode CC); in
  //  ARMISelLowering.cpp
  //  // TODO: Add support for conditions that handle floating point
  //  switch (ARMcc) {
  //    default:
  //      // printError("Unknown condition code");
  //      return;
  //    case ARMCC::EQ:
  //      Cmp = IRB->CreateICmpEQ(LHS, RHS);
  //      break;
  //    case ARMCC::NE:
  //      Cmp = IRB->CreateICmpNE(LHS, RHS);
  //      break;
  //    case ARMCC::HS:
  //      // HS - unsigned higher or same (or carry set)
  //      Cmp = IRB->CreateICmpUGE(LHS, RHS);
  //      break;
  //    case ARMCC::LO:
  //      // LO - unsigned lower (or carry clear)
  //      Cmp = IRB->CreateICmpULT(LHS, RHS);
  //      break;
  //    case ARMCC::MI:
  //      // MI - minus (negative)
  //      // printError("Condition code MI is not handled at this time!");
  //      return;
  //    // break;
  //    case ARMCC::PL:
  //      // PL - plus (positive or zero)
  //      // printError("Condition code PL is not handled at this time!");
  //      return;
  //    // break;
  //    case ARMCC::VS:
  //      // VS - V Set (signed overflow)
  //      // printError("Condition code VS is not handled at this time!");
  //      return;
  //    // break;
  //    case ARMCC::VC:
  //      // VC - V clear (no signed overflow)
  //      // printError("Condition code VC is not handled at this time!");
  //      return;
  //    // break;
  //    case ARMCC::HI:
  //      // HI - unsigned higher
  //      Cmp = IRB->CreateICmpUGT(LHS, RHS);
  //      break;
  //    case ARMCC::LS:
  //      // LS - unsigned lower or same
  //      Cmp = IRB->CreateICmpULE(LHS, RHS);
  //      break;
  //    case ARMCC::GE:
  //      // GE - signed greater or equal
  //      Cmp = IRB->CreateICmpSGE(LHS, RHS);
  //      break;
  //    case ARMCC::LT:
  //      // LT - signed less than
  //      Cmp = IRB->CreateICmpSLT(LHS, RHS);
  //      break;
  //    case ARMCC::GT:
  //      // GT - signed greater than
  //      Cmp = IRB->CreateICmpSGT(LHS, RHS);
  //      break;
  //    case ARMCC::LE:
  //      // LE - signed less than or equal
  //      Cmp = IRB->CreateICmpSLE(LHS, RHS);
  //      break;
  //  }
  //  (dyn_cast<Instruction>(Cmp))->setDebugLoc(N->getOperand(2)->getDebugLoc());
  //
  //  // Conditional branch
  //  Instruction *Br = IRB->CreateCondBr(Cmp, BBTgt, NextBB);
  //  Br->setDebugLoc(N->getDebugLoc());
  // return Br;
}
