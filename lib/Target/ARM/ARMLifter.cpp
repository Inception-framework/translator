#include "Target/ARM/ARMLifter.h"
#include "Target/ARM/ARMLifterManager.h"

using namespace llvm;

SDNode *ARMLifter::generic2OP(SDNode *N, uint16_t opcode) {

  SDValue OP0 = N->getOperand(0);
  SDValue OP1 = N->getOperand(1);

  EVT LdType = N->getValueType(0);
  SDVTList VTList = ARMLifterManager::DAG->getVTList(MVT::i32);
  SDLoc SL(N);

  SDValue res = ARMLifterManager::DAG->getNode(opcode, SL, VTList, OP0, OP1);

  for (auto j = 0; j < N->getNumValues(); j++) {
    ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, j), res);
  }

  // if (conditional == true && IREmitter::ack() == true) {
  // IREmitter::assign(res.getNode());
  // }

  return NULL;
}
