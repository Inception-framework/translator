
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

#include "Target/ARM/BarrierLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void BarrierLifter::registerLifter() {
  alm->registerLifter(this, std::string("BarrierLifter"), (unsigned)ARM::t2DMB,
                      (LifterHandler)&BarrierLifter::BarrierHandler);
  alm->registerLifter(this, std::string("BarrierLifter"), (unsigned)ARM::t2DSB,
                      (LifterHandler)&BarrierLifter::BarrierHandler);
  alm->registerLifter(this, std::string("BarrierLifter"), (unsigned)ARM::t2ISB,
                      (LifterHandler)&BarrierLifter::BarrierHandler);
}

void BarrierLifter::BarrierHandler(llvm::SDNode* N, IRBuilder<>* IRB) {
  // get the address
  uint32_t address =
      alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());

  // print info messages
  switch (N->getMachineOpcode()) {
    case ARM::t2DMB:
      inception_message(
          "[BarrierHandler] DMB found at address 0x%08x, treated as nop",
          address);
      break;
    case ARM::t2DSB:
      inception_message(
          "[BarrierHandler] DSB found at address 0x%08x, treated as nop",
          address);
      break;
    case ARM::t2ISB:
      inception_message(
          "[BarrierHandler] ISB found at address 0x%08x, treated as nop",
          address);
      break;
    default:
      inception_error(
          "[BarrierHandler] BARRIER found at address 0x%08x, type not "
          "supported",
          address);
  }
  Value* Res = ReadReg(Reg("R0"), IRB);
  saveNodeValue(N, Res);
}


