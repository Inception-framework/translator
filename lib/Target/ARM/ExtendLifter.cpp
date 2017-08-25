#include "Target/ARM/ExtendLifter.h"
#include "Target/ARM/ShiftLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void ExtendLifter::registerLifter() {
  // UXTB
  alm->registerLifter(this, std::string("ExtendLifter"), (unsigned)ARM::tUXTB,
                      (LifterHandler)&ExtendLifter::ExtendHandlerUXTB);
  alm->registerLifter(this, std::string("ExtendLifter"), (unsigned)ARM::t2UXTB,
                      (LifterHandler)&ExtendLifter::ExtendHandlerUXTB);
}

// UXTB register register (shift)
void ExtendLifter::ExtendHandlerUXTB(llvm::SDNode *N, llvm::IRBuilder<> *IRB) {
  // operands
  Value *Rm = visit(N->getOperand(0).getNode(), IRB);

  // possible rotation
  if (N->getMachineOpcode() == ARM::t2UXTB) {
    const ConstantSDNode *ShiftNode =
        dyn_cast<ConstantSDNode>(N->getOperand(1));
    if (!ShiftNode) {
      outs() << "ExtendHandler: Not a constant integer for rotate!\n";
      return;
    }
    uint32_t lsr_amount = 8 * ShiftNode->getZExtValue();
    uint32_t lsl_amount = 32 - lsr_amount;

    if (lsr_amount != 0) {
      Value *lsr =
          ConstantInt::get(alm->getContextRef(), APInt(32, lsr_amount, 10));
      Value *lsl =
          ConstantInt::get(alm->getContextRef(), APInt(32, lsl_amount, 10));

      Value *tmp1 = IRB->CreateLShr(Rm, lsr);
      Value *tmp2 = IRB->CreateShl(Rm, lsl);
      Rm = IRB->CreateOr(tmp1, tmp2);
    }
  }

  // operation
  Value *trunced =
      IRB->CreateTrunc(Rm, IntegerType::get(alm->getContextRef(), 8));
  Value *extended =
      IRB->CreateZExt(trunced, IntegerType::get(alm->getContextRef(), 32));

  alm->VisitMap[N] = extended;
}


