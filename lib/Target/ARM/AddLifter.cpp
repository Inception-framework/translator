#include "Target/ARM/AddLifter.h"

#include "ARMBaseInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;

SDNode *AddLifter::select(SDNode *N) {

  return AddLifter::generic2OP(N, ISD::ADD);
}
