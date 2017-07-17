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
}

void BranchLifter::BranchHandler(SDNode *N, IRBuilder<> *IRB) {
  IRB->CreateRetVoid();
}
