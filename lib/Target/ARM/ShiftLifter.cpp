#include "Target/ARM/ShiftLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void ShiftLifter::registerLifter() {
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2LSLri,
                      (LifterHandler)&ShiftLifter::ShiftHandler);
}

void ShiftLifter::ShiftHandler(SDNode *N, IRBuilder<> *IRB) {
  ARMSHIFTInfo *info = RetrieveGraphInformation(N, IRB);

  Instruction *Res =
      dyn_cast<Instruction>(IRB->CreateShl(info->Op0, info->Op1, info->Name));

  Res->setDebugLoc(N->getDebugLoc());

  alm->VisitMap[N] = Res;
}

ARMSHIFTInfo *ShiftLifter::RetrieveGraphInformation(SDNode *N,
                                                    IRBuilder<> *IRB) {
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);

  StringRef BaseName = getInstructionName(N, IRB);
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op0->getName());
  }

  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op1->getName());
  }

  StringRef Name = getIndexedValueName(BaseName);

  ARMSHIFTInfo *info = new ARMSHIFTInfo(Op0, Op1, Name);

  return info;
}
