
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

#include "Target/ARM/HintLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void HintLifter::registerLifter() {
  alm->registerLifter(this, std::string("HintLifter"), (unsigned)ARM::tHINT,
                      (LifterHandler)&HintLifter::HintHandler);
}

void HintLifter::HintHandler(llvm::SDNode* N, IRBuilder<>* IRB) {
  // get the type
  const ConstantSDNode* TypeNode = dyn_cast<ConstantSDNode>(N->getOperand(1));
  if (!TypeNode) {
    inception_error("[HintHandler] Type operand is not a constant integer");
  }
  int type = TypeNode->getZExtValue();

  uint32_t address =
      alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());

  // print info messages
  switch (type) {
    case 0:
      inception_warning(
          "[HintHandler] NOP found at address 0x%08x, make sure you are not "
          "decompiling after "
          "the end of the "
          "function",
          address);
      break;
    case 1:
      inception_message(
          "[HintHandler] YIELD found at address 0x%08x, treated as nop",
          address);
      break;
    case 2:
      inception_message(
          "[HintHandler] WFE found at address 0x%08x, treated as nop", address);
      break;
    case 3:
      inception_message(
          "[HintHandler] WFI found at address 0x%08x, treated as nop", address);
      break;
    case 4:
      inception_message(
          "[HintHandler] SEV found at address 0x%08x, treated as nop", address);
      break;
    case 5:
      inception_message(
          "[HintHandler] SEVL found at address 0x%08x, treated as nop",
          address);
      break;
    default:
      inception_error(
          "[HintHandler] tHINT found at address 0x%08x, type not supported",
          address);
  }

  Value* Res = ReadReg(Reg("R0"), IRB);
  saveNodeValue(N, Res);
}


