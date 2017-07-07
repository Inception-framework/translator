#include "Target/ARM/StoreLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

SDNode *StoreLifter::select(SDNode *N) {

  uint16_t TargetOpc = N->getMachineOpcode();

  if( TargetOpc == ARM::t2ADCri )
    return StoreLifter::generic2OP(N, ARMISD::ADDC);
  else
    return StoreLifter::generic2OP(N, ISD::ADD);
}
