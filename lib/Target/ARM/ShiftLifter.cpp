#include "Target/ARM/ShiftLifter.h"

#include "ARMBaseInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;

SDNode *ShiftLifter::select(SDNode *N) {

  return ShiftLifter::generic2OP(N, ISD::SHL);
}
