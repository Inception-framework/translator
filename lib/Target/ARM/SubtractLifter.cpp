#include "Target/ARM/SubtractLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"

using namespace llvm;
using namespace fracture;

void SubtractLifter::registerLifter() {
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::tSUBi3,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::tSUBi8,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::tSUBrr,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::tSUBspi,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2SUBrr,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2SUBri,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2SUBri12,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2SBCrr,
                      (LifterHandler)&SubtractLifter::SbcHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2SBCri,
                      (LifterHandler)&SubtractLifter::SbcHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2SUBSri,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2SUBSrr,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2SUBSrs,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2SUBrs,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"), (unsigned)ARM::tSBC,
                      (LifterHandler)&SubtractLifter::SbcHandler);
}

void SubtractLifter::SubHandler(llvm::SDNode *N, llvm::IRBuilder<> *IRB) {
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);
  Value *cf = getConstant("1");

  unsigned opcode = N->getMachineOpcode();
  switch (opcode) {
    case ARM::tSUBspi:
      Op1 = IRB->CreateMul(Op1, getConstant("4"));
  }

  // subtraction
  Value *onescompl = IRB->CreateNot(Op1);
  Value *Res_add = IRB->CreateAdd(Op0, onescompl);
  Value *Res = IRB->CreateAdd(Res_add, cf);

  if (IsSetFlags(N)) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));
    // Compute NF.
    flags->WriteNF(IRB, Res);
    // Compute ZF.
    flags->WriteZF(IRB, Res);
    // Ccompute VF.
    flags->WriteVFAdd(IRB, Res_add, Op0, onescompl);
    flags->WriteVFAdc(IRB, Res, Res_add, cf);
    // Compute CF.
    flags->WriteCFAdd(IRB, Res_add, Op0);
    flags->WriteCFAdc(IRB, Res, Res_add);
  }

  saveNodeValue(N, Res);
}

void SubtractLifter::SbcHandler(llvm::SDNode *N, llvm::IRBuilder<> *IRB) {
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);
  Value *cf = ReadReg(Reg("CF"), IRB);

  // subtraction
  Value *onescompl = IRB->CreateNot(Op1);
  Value *Res_add = IRB->CreateAdd(Op0, onescompl);
  Value *Res = IRB->CreateAdd(Res_add, cf);

  if (IsSetFlags(N)) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));
    // Compute NF.
    flags->WriteNF(IRB, Res);
    // Compute ZF.
    flags->WriteZF(IRB, Res);
    // Ccompute VF.
    flags->WriteVFAdd(IRB, Res_add, Op0, onescompl);
    flags->WriteVFAdc(IRB, Res, Res_add, cf);
    // Compute CF.
    flags->WriteCFAdd(IRB, Res_add, Op0);
    flags->WriteCFAdc(IRB, Res, Res_add);
  }

  saveNodeValue(N, Res);
}
