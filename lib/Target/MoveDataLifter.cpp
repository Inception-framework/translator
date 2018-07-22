
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

#include "Target/ARM/MoveDataLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"
#include "Target/ARM/ShiftLifter.h"

using namespace llvm;
using namespace fracture;

void MoveDataLifter::registerLifter() {
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MOVi16,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MOVi,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::tMOVSr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::tMOVi8,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::tMOVr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MOVr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MOVr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MOVi,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::t2MOVTi16,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNCCi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNr,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNsi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNsr,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MVNr,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MVNi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::tMVN,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MVNs,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
}

void MoveDataLifter::MoveHandler(llvm::SDNode* N, IRBuilder<>* IRB) {

  Value* Op0 = visit(N->getOperand(0).getNode(), IRB);

  if (IsSetFlags(N)) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter* flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    ////Compute NF
    // flags->WriteNFAdd(IRB, Res);
    // Compute NF.
    flags->WriteNF(IRB, Op0);
    // Compute ZF.
    flags->WriteZF(IRB, Op0);
    // compute CF
    if (N->getMachineOpcode() == ARM::t2MOVi) {
      const ConstantSDNode* ConstNode =
          dyn_cast<ConstantSDNode>(N->getOperand(0));
      if (ConstNode) {
        uint32_t constant = ConstNode->getZExtValue();
        flags->WriteCFconstant(IRB, constant);
      }
    }
  }

  saveNodeValue(N, Op0);
}

void MoveDataLifter::MoveNotHandler(llvm::SDNode* N, IRBuilder<>* IRB) {

  Value* Op0 = NULL;
  unsigned opcode = N->getMachineOpcode();
  if (opcode == ARM::t2MVNs) {
    ShiftLifter* shiftLifter = dyn_cast<ShiftLifter>(alm->resolve("SHIFT"));
    shiftLifter->ShiftHandlerShiftOp(N, IRB);
    Op0 = getSavedValue(N);
  } else {
    Op0 = visit(N->getOperand(0).getNode(), IRB);
  }

  Value* Res = IRB->CreateNot(Op0);

  if (IsSetFlags(N)) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter* flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    ////Compute NF
    // flags->WriteNFAdd(IRB, Res);
    // Compute NF.
    flags->WriteNF(IRB, Res);
    // Compute ZF.
    flags->WriteZF(IRB, Res);
    // Compute CF
    if (opcode == ARM::t2MVNi) {
      const ConstantSDNode* ConstNode =
          dyn_cast<ConstantSDNode>(N->getOperand(0));
      if (ConstNode) {
        uint32_t constant = ConstNode->getZExtValue();
        flags->WriteCFconstant(IRB, constant);
      } else {
        flags->WriteCFShiftL(IRB, Op0, getConstant("0"));
      }
    }
  }

  saveNodeValue(N, Res);
}
