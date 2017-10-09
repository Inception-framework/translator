#include "Target/ARM/BranchLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include <vector>

#include "Utils/Builder.h"
#include "Utils/IContext.h"

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
  saveNodeValue(N, Ret);
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

  // create the code that check the condition
  // NULL if condition is AL
  int cond = CCNode->getZExtValue();
  Value *Cmp = createCondition(cond, IRB);

  // Unconditional branch or inside it block (last instruction)
  if (Cmp == NULL || (IContext::alm->Dec->it_state & 0b1111) != 0) {
    if(BBTgt == NULL) {
      CreateCall(N, IRB, Tgt);
      Instruction* Ret = IRB->CreateRetVoid();
      Ret->setDebugLoc(N->getDebugLoc());
      saveNodeValue(N, Ret);
    }else {
      Instruction *Br = IRB->CreateBr(BBTgt);
      Br->setDebugLoc(N->getDebugLoc());
      saveNodeValue(N, Br);
    }
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

  // Conditional branch
  Instruction *Br = IRB->CreateCondBr(Cmp, BBTgt, NextBB);
  Br->setDebugLoc(N->getDebugLoc());
  saveNodeValue(N, Br);
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
  CreateCall(N, IRB, Tgt);
}

// simple indirect call promotion
void BranchLifter::BranchHandlerBLXr(SDNode *N, IRBuilder<> *IRB) {
  // pointer register
  Value *Rm = visit(N->getOperand(3).getNode(), IRB);

  Constant *func_ptr = GetIntFunctionPointer("inception_icp");

  IRB->CreateCall(func_ptr, Rm);
  Value *dummyLR = getConstant("0");
  saveNodeValue(N, dummyLR);
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
  saveNodeValue(N, Br);
  return;
}
