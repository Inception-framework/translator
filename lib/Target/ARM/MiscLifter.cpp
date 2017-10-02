#include "Target/ARM/MiscLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void MiscLifter::registerLifter() {
  alm->registerLifter(this, std::string("MiscLifter"), (unsigned)ARM::t2RBIT,
                      (LifterHandler)&MiscLifter::RBITHandler);
}

void MiscLifter::RBITHandler(llvm::SDNode* N, IRBuilder<>* IRB) {
  Value* Op0 = visit(N->getOperand(0).getNode(), IRB);

  Value* Bit = getConstant("0");
  Value* Res = getConstant("0");
  for (int i = 0; i < 32; i++) {
    Bit = IRB->CreateAnd(Op0, getConstant(1 << i));
    int shift = 31 - 2 * i;
    if (shift >= 0) {
      Bit = IRB->CreateShl(Bit, getConstant(shift));
    } else {
      Bit = IRB->CreateLShr(Bit, getConstant(-shift));
    }
    Res = IRB->CreateOr(Res, Bit);
  }

  saveNodeValue(N, Res);
}
