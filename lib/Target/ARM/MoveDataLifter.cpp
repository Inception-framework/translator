#include "Target/ARM/MoveDataLifter.h"

#include "ARMBaseInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "Target/ARM/ARMLifterManager.h"

using namespace llvm;

SDNode *MoveDataLifter::select(SDNode *N) {

  SDNode *C2R = NULL;
  for (SDNode::use_iterator I = N->use_begin(), E = N->use_end(); I != E; ++I) {
    if (I->getOpcode() == ISD::CopyToReg) {
      C2R = *I;
      break;
    }
  }
  assert(C2R && "Move instruction without CopytoReg!");
  C2R->setDebugLoc(N->getDebugLoc());

  for (auto j = 0; j < N->getNumValues(); j++) {
    for (auto i = 0; i < N->getNumOperands(); i++) {
      ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, j),
                                                       N->getOperand(i));
    }
  }

  return NULL;
}
