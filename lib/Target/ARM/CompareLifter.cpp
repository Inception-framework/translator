#include "Target/ARM/CompareLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

SDNode *CompareLifter::select(SDNode *N) {

  return CompareLifter::generic2OP(N, ARMISD::CMP);
}
