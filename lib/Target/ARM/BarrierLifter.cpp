#include "Target/ARM/BarrierLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void BarrierLifter::registerLifter() {
  alm->registerLifter(this, std::string("BarrierLifter"), (unsigned)ARM::t2DMB,
                      (LifterHandler)&BarrierLifter::BarrierHandler);
  alm->registerLifter(this, std::string("BarrierLifter"), (unsigned)ARM::t2DSB,
                      (LifterHandler)&BarrierLifter::BarrierHandler);
  alm->registerLifter(this, std::string("BarrierLifter"), (unsigned)ARM::t2ISB,
                      (LifterHandler)&BarrierLifter::BarrierHandler);
}

void BarrierLifter::BarrierHandler(llvm::SDNode* N, IRBuilder<>* IRB) {
  // get the address
  uint32_t address =
      alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());

  // print info messages
  switch (N->getMachineOpcode()) {
    case ARM::t2DMB:
      inception_message(
          "[BarrierHandler] DMB found at address 0x%08x, treated as nop",
          address);
      break;
    case ARM::t2DSB:
      inception_message(
          "[BarrierHandler] DSB found at address 0x%08x, treated as nop",
          address);
      break;
    case ARM::t2ISB:
      inception_message(
          "[BarrierHandler] ISB found at address 0x%08x, treated as nop",
          address);
      break;
    default:
      inception_error(
          "[BarrierHandler] BARRIER found at address 0x%08x, type not "
          "supported",
          address);
  }
}


