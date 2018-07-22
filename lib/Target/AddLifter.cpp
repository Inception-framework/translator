
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

#include "Target/ARM/AddLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"
#include "Target/ARM/ShiftLifter.h"

#include "Utils/Builder.h"

using namespace llvm;
using namespace fracture;

void AddLifter::registerLifter() {
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDrr,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDhirr,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDrSPi,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDspi,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDi8,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDframe,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDi3,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDrSP,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDspr,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDSri,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDSrr,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDSrs,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDri,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDri12,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDrr,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDrs,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADCri,
                      (LifterHandler)&AddLifter::AdcHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADCrr,
                      (LifterHandler)&AddLifter::AdcHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADCrs,
                      (LifterHandler)&AddLifter::AdcHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADC,
                      (LifterHandler)&AddLifter::AdcHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADR,
                      (LifterHandler)&AddLifter::AdrHandler);
  //  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADR,
  //                      (LifterHandler)&AddLifter::AdrHandler);
}

void AddLifter::AdcHandler(SDNode *N, IRBuilder<> *IRB) {
  auto cf = ReadReg(Reg("CF"), IRB);

  AddHandler(N, IRB);  // Si opérande à la même position
  Value *Res_add = getSavedValue(N);

  // then
  Value *Res_adc = IRB->CreateAdd(Res_add, cf);

  bool S = IsSetFlags(N);
  if (S) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    // Compute NF.
    flags->WriteNF(IRB, Res_adc);
    // Compute ZF.
    flags->WriteZF(IRB, Res_adc);
    // Ccompute VF.
    flags->WriteVFAdc(IRB, Res_adc, Res_add, cf);
    // Compute CF.
    flags->WriteCFAdc(IRB, Res_adc, Res_add);
  }

  saveNodeValue(N, Res_adc);
}

void AddLifter::AddHandler(SDNode *N, IRBuilder<> *IRB) {
  ARMADDInfo *info = RetrieveGraphInformation(N, IRB);

  switch (N->getMachineOpcode()) {
    case ARM::tADDspi:
    case ARM::tADDrSPi:
      info->Op1 = IRB->CreateShl(info->Op1, getConstant("2"));
  }

  Value* Res = IRB->CreateAdd(info->Op0, info->Op1);

  if (info->S) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    ////Compute NF
    // flags->WriteNFAdd(IRB, Res);
    // Compute NF.
    flags->WriteNF(IRB, Res);
    // Compute ZF.
    flags->WriteZF(IRB, Res);
    // Ccompute VF.
    flags->WriteVFAdd(IRB, Res, info->Op0, info->Op1);
    // Compute CF.
    flags->WriteCFAdd(IRB, Res, info->Op0);
  }

  saveNodeValue(N, Res);
}

void AddLifter::AdrHandler(SDNode *N, IRBuilder<> *IRB) {
  Value *Op0 = NULL;
  Value *Res = NULL;

  switch (N->getMachineOpcode()) {
    case ARM::tADR: {
      const ConstantSDNode *immediateNode =
          dyn_cast<ConstantSDNode>(N->getOperand(0));
      if (!immediateNode) {
        inception_error("[AdrHandler]: Not a constant integer for immediate");
      }

      uint32_t immediate = immediateNode->getZExtValue() << 2;

      uint64_t pc =
          alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
      pc += 4;     // pipeline
      pc &= ~0x3;  // align(address,4)
      Res = getInteger(pc + immediate);
    }
  }

  saveNodeValue(N, Res);
}

ARMADDInfo *AddLifter::RetrieveGraphInformation(SDNode *N, IRBuilder<> *IRB) {
  Value *Op1 = NULL;
  Value *Op0 = NULL;

  Op0 = visit(N->getOperand(0).getNode(), IRB);

  unsigned opcode = N->getMachineOpcode();
  switch (opcode) {
    case ARM::t2ADDSrs:
    case ARM::t2ADDrs:
    case ARM::t2ADCrs: {
      ShiftLifter *shiftLifter = dyn_cast<ShiftLifter>(alm->resolve("SHIFT"));
      shiftLifter->ShiftHandlerShiftOp(N, IRB);
      Op1 = getSavedValue(N);
      break;
    }
    default:
      Op1 = visit(N->getOperand(1).getNode(), IRB);
  }

  bool S = IsSetFlags(N);
  ARMADDInfo *info = new ARMADDInfo(Op0, Op1, S);

  return info;
}
