#include "Target/ARM/DummyLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void DummyLifter::registerLifter() {
  // alm->registerLifter(ADDLifter, ARM::tADDrr, ADDHandler);
}

void DummyLifter::handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  visit(N, IRB);
}
