#include "Target/ARM/LogicalLifter.h"
#include "Target/ARM/FlagsLifter.h"
#include "Target/ARM/ShiftLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void LogicalLifter::registerLifter() {
  // TST
  alm->registerLifter(this, std::string("LogicalLifter"), (unsigned)ARM::tTST,
                      (LifterHandler)&LogicalLifter::TstHandlerRR);
  alm->registerLifter(this, std::string("LogicalLifter"),
                      (unsigned)ARM::t2TSTri,
                      (LifterHandler)&LogicalLifter::TstHandlerRI);
  alm->registerLifter(this, std::string("LogicalLifter"),
                      (unsigned)ARM::t2TSTrr,
                      (LifterHandler)&LogicalLifter::TstHandlerRR);
  alm->registerLifter(this, std::string("LogicalLifter"),
                      (unsigned)ARM::t2TSTrs,
                      (LifterHandler)&LogicalLifter::TstHandlerRS);
  // TEQ
  alm->registerLifter(this, std::string("LogicalLifter"),
                      (unsigned)ARM::t2TEQri,
                      (LifterHandler)&LogicalLifter::TstHandlerRI);
  alm->registerLifter(this, std::string("LogicalLifter"),
                      (unsigned)ARM::t2TEQrr,
                      (LifterHandler)&LogicalLifter::TstHandlerRR);
  alm->registerLifter(this, std::string("LogicalLifter"),
                      (unsigned)ARM::t2TEQrs,
                      (LifterHandler)&LogicalLifter::TstHandlerRS);
}

// TST / TEQ register immediate
void LogicalLifter::TstHandlerRI(llvm::SDNode *N, llvm::IRBuilder<> *IRB) {
  // operands
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);

  const ConstantSDNode *ConstNode = dyn_cast<ConstantSDNode>(N->getOperand(1));
  if (!ConstNode) {
    outs() << "TstHandler: Not a constant integer for immediate!\n";
    return;
  }

  uint32_t constant = ConstNode->getZExtValue();

  // operation
  Value *Res = NULL;
  switch (N->getMachineOpcode()) {
    case ARM::t2TSTri:
      Res = IRB->CreateAnd(Op0, Op1);
      break;
    case ARM::t2TEQri:
      Res = IRB->CreateXor(Op0, Op1);
      break;
    default:
      outs() << "TstHandler unsupported opcode\n";
      return;
  }

  // Write the flag updates.
  // Compute AF.
  FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));
  // Compute NF.
  flags->WriteNF(IRB, Res);
  // Compute ZF.
  flags->WriteZF(IRB, Res);
  // Compute CF.
  flags->WriteCFconstant(IRB, constant);

  // Dummy CPSR, not used, Flags are used instead if necessary
  Value *dummyCPSR = getConstant("0");

  alm->VisitMap[N] = dummyCPSR;
}

// TST / TEQ register register
void LogicalLifter::TstHandlerRR(llvm::SDNode *N, llvm::IRBuilder<> *IRB) {
  // operands
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);

  // operation
  Value *Res = NULL;
  switch (N->getMachineOpcode()) {
    case ARM::tTST:
    case ARM::t2TSTrr:
      Res = IRB->CreateAnd(Op0, Op1);
      break;
    case ARM::t2TEQrr:
      Res = IRB->CreateXor(Op0, Op1);
      break;
    default:
      outs() << "TstHandler unsupported opcode\n";
      return;
  }

  // Write the flag updates.
  // Compute AF.
  FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));
  // Compute NF.
  flags->WriteNF(IRB, Res);
  // Compute ZF.
  flags->WriteZF(IRB, Res);
  // Compute CF.
  flags->WriteCFShiftL(IRB, Op1, getConstant("0"));

  // Dummy CPSR, not used, Flags are used instead if necessary
  Value *dummyCPSR = getConstant("0");

  alm->VisitMap[N] = dummyCPSR;
}

// TST register shift
void LogicalLifter::TstHandlerRS(llvm::SDNode *N, llvm::IRBuilder<> *IRB) {
  // operands
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);

  // shift operation
  ShiftLifter *shiftLifter = dyn_cast<ShiftLifter>(alm->resolve("FLAGS"));
  shiftLifter->ShiftHandlerShiftOp(N, IRB);
  Value *shifted = alm->VisitMap[N];

  // operation
  Value *Res = NULL;
  switch (N->getMachineOpcode()) {
    case ARM::t2TSTrs:
      Res = IRB->CreateAnd(Op0, shifted);
      break;
    case ARM::t2TEQrs:
      Res = IRB->CreateXor(Op0, shifted);
      break;
    default:
      outs() << "TstHandler unsupported opcode\n";
      return;
  }

  // Write the flag updates.
  // Compute AF.
  FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));
  // Compute NF.
  flags->WriteNF(IRB, Res);
  // Compute ZF.
  flags->WriteZF(IRB, Res);
  // Compute CF.
  // computed by the shift

  // Dummy CPSR, not used, Flags are used instead if necessary
  Value *dummyCPSR = getConstant("0");

  alm->VisitMap[N] = dummyCPSR;
}
