#include "Target/ARM/ShiftLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void ShiftLifter::registerLifter() {
  // LSL rd,rm,<rs|sh>
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2LSLri,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSL);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2LSLrr,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSL);
  // LSR rd,rm,<rs|sh>
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2LSRri,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSR);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2LSRrr,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSR);
  // ASR rd,rm,<rs|sh>
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2ASRri,
                      (LifterHandler)&ShiftLifter::ShiftHandlerASR);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2ASRrr,
                      (LifterHandler)&ShiftLifter::ShiftHandlerASR);
  // ROR rd,rm,<rs|sh>
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2RORri,
                      (LifterHandler)&ShiftLifter::ShiftHandlerROR);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2RORrr,
                      (LifterHandler)&ShiftLifter::ShiftHandlerROR);
}

void ShiftLifter::ShiftHandlerLSL(SDNode *N, IRBuilder<> *IRB) {
  ARMSHIFTInfo *info = RetrieveGraphInformation(N, IRB);

  Instruction *Res =
      dyn_cast<Instruction>(IRB->CreateShl(info->Op0, info->Op1));

  Res->setDebugLoc(N->getDebugLoc());

  alm->VisitMap[N] = Res;
}

// note1: shift in two parts, because llvm does not allow shifting a 32-bit
// value
// by 32 bit
// note2: the range is 1-32. In case of immediate, 32 is encoded as 0.
void ShiftLifter::ShiftHandlerLSR(SDNode *N, IRBuilder<> *IRB) {
  ARMSHIFTInfo *info = RetrieveGraphInformation(N, IRB);

  Value *const_31 = getConstant("31");

  Value *const_1 = getConstant("1");

  Value *const_0 = getConstant("0");

  Value *shift_amount_min1;
  if (info->Op1 == const_0) {
    shift_amount_min1 = IRB->CreateAdd(info->Op1, const_31);
  } else {
    shift_amount_min1 = IRB->CreateSub(info->Op1, const_1);
  }

  Value *partial_res;
  partial_res = IRB->CreateLShr(info->Op0, shift_amount_min1);

  Instruction *Res =
      dyn_cast<Instruction>(IRB->CreateLShr(partial_res, const_1));

  Res->setDebugLoc(N->getDebugLoc());

  alm->VisitMap[N] = Res;
}

// note1: shift in two parts, because llvm does not allow shifting a 32-bit
// value
// by 32 bit
// note2: the range is 1-32. In case of immediate, 32 is encoded as 0.
void ShiftLifter::ShiftHandlerASR(SDNode *N, IRBuilder<> *IRB) {
  ARMSHIFTInfo *info = RetrieveGraphInformation(N, IRB);

  Value *const_31 = getConstant("31");

  Value *const_1 = getConstant("1");

  Value *const_0 = getConstant("0");

  Value *shift_amount_min1;
  // StringRef BaseName = getBaseValueName(shift_amount_min1->getName());
  // StringRef Name = getIndexedValueName(BaseName);
  if (info->Op1 == const_0) {
    shift_amount_min1 = IRB->CreateAdd(info->Op1, const_31);
  } else {
    shift_amount_min1 = IRB->CreateSub(info->Op1, const_1);
  }

  Value *partial_res;
  // BaseName = getBaseValueName(partial_res->getName());
  // Name = getIndexedValueName(BaseName);
  partial_res = IRB->CreateAShr(info->Op0, shift_amount_min1);

  Instruction *Res =
      dyn_cast<Instruction>(IRB->CreateAShr(partial_res, const_1));

  Res->setDebugLoc(N->getDebugLoc());

  alm->VisitMap[N] = Res;
}

// Note: llvm does not have ror
void ShiftLifter::ShiftHandlerROR(SDNode *N, IRBuilder<> *IRB) {
  ARMSHIFTInfo *info = RetrieveGraphInformation(N, IRB);

  Value *const_32 = getConstant("32");

  Value *lshift_amount = IRB->CreateSub(const_32, info->Op1);
  Value *high = IRB->CreateShl(info->Op0, lshift_amount);
  Value *low = IRB->CreateLShr(info->Op0, info->Op1);

  Instruction *Res =
      dyn_cast<Instruction>(IRB->CreateOr(high, low));

  Res->setDebugLoc(N->getDebugLoc());

  alm->VisitMap[N] = Res;
}

ARMSHIFTInfo *ShiftLifter::RetrieveGraphInformation(SDNode *N,
                                                    IRBuilder<> *IRB) {
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);

  ARMSHIFTInfo *info = new ARMSHIFTInfo(Op0, Op1);

  return info;
}
