#include "Target/ARM/ITLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifter.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"

using namespace llvm;
using namespace fracture;

void ITLifter::registerLifter() {
  alm->registerLifter(this, std::string("ITLifter"), (unsigned)ARM::t2IT,
                      (LifterHandler)&ITLifter::ITHandler);
}

void ITLifter::ITHandler(const SDNode* N, IRBuilder<>* IRB) {
  // get or create ITSTATE reg
  Reg("ITSTATE");

  // get ITSTATE[7:5]
  const ConstantSDNode* CondNode = dyn_cast<ConstantSDNode>(N->getOperand(0));
  if (!CondNode) {
    outs() << "[ITHandler] Not a constant integer for cond!\n";
    return;
  }
  uint32_t cond = CondNode->getSExtValue();

  // get ITSTATE[4:0]
  const ConstantSDNode* MaskNode = dyn_cast<ConstantSDNode>(N->getOperand(1));
  if (!MaskNode) {
    outs() << "[ITHandler] Not a constant integer for mask!\n";
    return;
  }
  uint32_t mask = MaskNode->getSExtValue();
  alm->Dec->it_true = (cond & 1);

  // set ITSTATE value
  uint32_t itstate = (cond << 4) | mask;
  Value* ITSTATE =
      ConstantInt::get(IContext::getContextRef(), APInt(32, itstate, 10));

  alm->Dec->it_state = itstate;
  // outs() << "setting it_state: " << itstate << "\n";

  saveNodeValue(N, ITSTATE);
}
