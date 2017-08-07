#include "Target/ARM/SubtractLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void SubtractLifter::registerLifter() {
  alm->registerLifter(this, std::string("SubLifter"), (unsigned)ARM::tSUBi3,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubLifter"), (unsigned)ARM::tSUBi8,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubLifter"), (unsigned)ARM::tSUBrr,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubLifter"), (unsigned)ARM::tSUBspi,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubLifter"), (unsigned)ARM::t2SUBrr,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubLifter"), (unsigned)ARM::t2SUBri,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubLifter"), (unsigned)ARM::t2SUBri12,
                      (LifterHandler)&SubtractLifter::SubHandler);
}

void SubtractLifter::SubHandler(SDNode *N, IRBuilder<> *IRB) {
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);

  Instruction *Res =
      dyn_cast<Instruction>(IRB->CreateSub(Op0, Op1));

  saveNodeValue(N, Res);
}
