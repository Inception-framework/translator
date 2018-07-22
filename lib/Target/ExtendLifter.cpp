
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

#include "Target/ARM/ExtendLifter.h"
#include "Target/ARM/ShiftLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
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
  // UXTH
  alm->registerLifter(this, std::string("ExtendLifter"), (unsigned)ARM::tUXTH,
                      (LifterHandler)&ExtendLifter::ExtendHandlerUXTB);
  alm->registerLifter(this, std::string("ExtendLifter"), (unsigned)ARM::t2UXTH,
                      (LifterHandler)&ExtendLifter::ExtendHandlerUXTB);
  // SXTB
  alm->registerLifter(this, std::string("ExtendLifter"), (unsigned)ARM::tSXTB,
                      (LifterHandler)&ExtendLifter::ExtendHandlerUXTB);
  alm->registerLifter(this, std::string("ExtendLifter"), (unsigned)ARM::t2SXTB,
                      (LifterHandler)&ExtendLifter::ExtendHandlerUXTB);
  // SXTH
  alm->registerLifter(this, std::string("ExtendLifter"), (unsigned)ARM::tSXTH,
                      (LifterHandler)&ExtendLifter::ExtendHandlerUXTB);
  alm->registerLifter(this, std::string("ExtendLifter"), (unsigned)ARM::t2SXTH,
                      (LifterHandler)&ExtendLifter::ExtendHandlerUXTB);
}

// UXTB register register (shift)
void ExtendLifter::ExtendHandlerUXTB(llvm::SDNode *N, llvm::IRBuilder<> *IRB) {
  // operands
  Value *Rm = visit(N->getOperand(0).getNode(), IRB);

  // possible rotation
  switch (N->getMachineOpcode()) {
    case ARM::t2UXTB:
    case ARM::t2SXTB:
    case ARM::t2UXTH:
    case ARM::t2SXTH:
      const ConstantSDNode *ShiftNode =
          dyn_cast<ConstantSDNode>(N->getOperand(1));
      if (!ShiftNode) {
        outs() << "ExtendHandler: Not a constant integer for rotate!\n";
        return;
      }
      uint32_t lsr_amount = 8 * ShiftNode->getZExtValue();
      uint32_t lsl_amount = 32 - lsr_amount;

      if (lsr_amount != 0) {
        Value *lsr = ConstantInt::get(IContext::getContextRef(),
                                      APInt(32, lsr_amount, 10));
        Value *lsl = ConstantInt::get(IContext::getContextRef(),
                                      APInt(32, lsl_amount, 10));

        Value *tmp1 = IRB->CreateLShr(Rm, lsr);
        Value *tmp2 = IRB->CreateShl(Rm, lsl);
        Rm = IRB->CreateOr(tmp1, tmp2);
      }
      break;
  }

  int size = 0;
  bool sign = false;
  switch (N->getMachineOpcode()) {
    case ARM::tUXTB:
    case ARM::t2UXTB:
      size = 8;
      sign = false;
      break;
    case ARM::tUXTH:
    case ARM::t2UXTH:
      size = 16;
      sign = false;
      break;
    case ARM::tSXTB:
    case ARM::t2SXTB:
      size = 8;
      sign = true;
      break;
    case ARM::tSXTH:
    case ARM::t2SXTH:
      size = 16;
      sign = true;
      break;
  }

  // operation
  Value *trunced = IRB->CreateTrunc(
      Rm, IntegerType::get(IContext::getContextRef(), size));
  Value *extended = NULL;

  if (sign) {
    extended = IRB->CreateSExt(
        trunced, IntegerType::get(IContext::getContextRef(), 32));
  } else {
    extended = IRB->CreateZExt(
        trunced, IntegerType::get(IContext::getContextRef(), 32));
  }

  saveNodeValue(N, extended);
}
