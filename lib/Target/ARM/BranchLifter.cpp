#include "Target/ARM/BranchLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

SDNode *BranchLifter::select(SDNode *N) {

  SDValue Chain = SDValue(N->getOperand(1).getNode(), 1);

  SDLoc SL(N);

  SDVTList VTList = ARMLifterManager::DAG->getVTList(MVT::i32, MVT::Other);

  SDValue CallNode =
      ARMLifterManager::DAG->getNode(ARMISD::RET_FLAG, SL, VTList, Chain);

  ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(
      SDValue(N->getOperand(0).getNode(), 1), SDValue(CallNode.getNode(), 0));

  ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(
      SDValue(N, 0), SDValue(CallNode.getNode(), 1));
  return NULL;
}
