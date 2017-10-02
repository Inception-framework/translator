#include "Target/ARM/DivLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Utils/Builder.h"

using namespace llvm;
using namespace fracture;

void DivLifter::registerLifter() {
  alm->registerLifter(this, std::string("DivLifter"), (unsigned)ARM::t2UDIV,
                      (LifterHandler)&DivLifter::UDIVHandler);
}

void DivLifter::UDIVHandler(SDNode *N, IRBuilder<> *IRB) {
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);

  Value *Res = IRB->CreateUDiv(Op0, Op1);

  saveNodeValue(N, Res);
}
