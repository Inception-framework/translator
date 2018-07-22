
/*
    This file is part of Inception translator.

    Inception translator is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Inception translator.  If not, see <https://www.gnu.org/licenses/>.

    Copyright (c) 2017 Maxim Integrated, Inc.
    Author: Nassim Corteggiani <nassim.corteggiani@maximintegrated.com>

    Copyright (c) 2017 EURECOM, Inc.
    Author: Giovanni Camurati <giovanni.camurati@eurecom.fr>
*/

#include "Target/ARM/SubtractLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"
#include "Target/ARM/ShiftLifter.h"

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
                      (unsigned)ARM::t2SBCrs,
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
  alm->registerLifter(this, std::string("SubtractLifter"), (unsigned)ARM::tRSB,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2RSBri,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2RSBrr,
                      (LifterHandler)&SubtractLifter::SubHandler);
  alm->registerLifter(this, std::string("SubtractLifter"),
                      (unsigned)ARM::t2RSBrs,
                      (LifterHandler)&SubtractLifter::SubHandler);


}

void SubtractLifter::SubHandler(llvm::SDNode *N, llvm::IRBuilder<> *IRB) {
  Value *Op0 = NULL;

  Value *Op1 = NULL;

  unsigned opcode = N->getMachineOpcode();
  switch (opcode) {
    case ARM::t2SUBSrs:
    case ARM::t2SUBrs: {
      Op0 = visit(N->getOperand(0).getNode(), IRB);
      ShiftLifter *shiftLifter = dyn_cast<ShiftLifter>(alm->resolve("SHIFT"));
      shiftLifter->ShiftHandlerShiftOp(N, IRB);
      Op1 = getSavedValue(N);
      break;
    }
    case ARM::tRSB:
      Op0 = getConstant("0");
      Op1 = visit(N->getOperand(0).getNode(), IRB);
      break;
    case ARM::t2RSBri:
      Op0 = visit(N->getOperand(1).getNode(), IRB);
      Op1 = visit(N->getOperand(0).getNode(), IRB);
      break;
    default:
      Op0 = visit(N->getOperand(0).getNode(), IRB);
      Op1 = visit(N->getOperand(1).getNode(), IRB);
  }

  Value *cf = getConstant("1");

  opcode = N->getMachineOpcode();
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
  Value *Op1 = NULL;

  Value *cf = ReadReg(Reg("CF"), IRB);

  unsigned opcode = N->getMachineOpcode();
  switch (opcode) {
    case ARM::t2SBCrs: {
      ShiftLifter *shiftLifter = dyn_cast<ShiftLifter>(alm->resolve("SHIFT"));
      shiftLifter->ShiftHandlerShiftOp(N, IRB);
      Op1 = getSavedValue(N);
      break;
    }
    default:
      Op1 = visit(N->getOperand(1).getNode(), IRB);
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
