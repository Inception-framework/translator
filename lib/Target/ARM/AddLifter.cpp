#include "Target/ARM/AddLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

SDNode *AddLifter::select(SDNode *N) {

  uint16_t TargetOpc = N->getMachineOpcode();

  if( TargetOpc == ARM::t2ADCri )
    return AddLifter::generic2OP(N, ARMISD::ADDC);
  else
    return AddLifter::generic2OP(N, ISD::ADD);
}
