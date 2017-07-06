#include "Target/ARM/SubtractLifter.h"

#include "ARMBaseInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;

SDNode *SubLifter::select(SDNode *N) {

  return SubLifter::generic2OP(N, ISD::SUB);
}
