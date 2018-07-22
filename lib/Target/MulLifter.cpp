
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

#include "Target/ARM/MulLifter.h"

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

void MulLifter::registerLifter() {
  alm->registerLifter(this, std::string("MulLifter"), (unsigned)ARM::t2MUL,
                      (LifterHandler)&MulLifter::MulHandler);
  alm->registerLifter(this, std::string("MulLifter"), (unsigned)ARM::tMUL,
                      (LifterHandler)&MulLifter::MulHandler);
  alm->registerLifter(this, std::string("MulLifter"), (unsigned)ARM::t2UMLAL,
                      (LifterHandler)&MulLifter::UmlalHandler);
}

void MulLifter::MulHandler(SDNode *N, IRBuilder<> *IRB) {
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);

  Value *Res = IRB->CreateMul(Op0, Op1);

  saveNodeValue(N, Res);
}

void MulLifter::UmlalHandler(SDNode *N, IRBuilder<> *IRB) {
  Value *x = visit(N->getOperand(0).getNode(), IRB);
  Value *y = visit(N->getOperand(1).getNode(), IRB);
  Value *Z_lo = visit(N->getOperand(2).getNode(), IRB);
  Value *Z_hi = visit(N->getOperand(3).getNode(), IRB);

  Value *x_lo = IRB->CreateAnd(x, getConstant(0x0000ffff));
  Value *x_hi = IRB->CreateAnd(x, getConstant(0xffff0000));
  x_hi = IRB->CreateLShr(x_hi, getConstant(16));

  Value *y_lo = IRB->CreateAnd(y, getConstant(0x0000ffff));
  Value *y_hi = IRB->CreateAnd(y, getConstant(0xffff0000));
  y_hi = IRB->CreateLShr(y_hi, getConstant("16"));

  Value *m0 = IRB->CreateMul(x_lo, y_lo);
  Value *m1 = IRB->CreateMul(x_lo, y_hi);
  Value *m2 = IRB->CreateMul(x_hi, y_lo);
  Value *m3 = IRB->CreateMul(x_hi, y_hi);

  Value *m1_left = IRB->CreateAnd(m1, getConstant(0xffff0000));
  m1_left = IRB->CreateLShr(m1_left, getConstant("16"));

  Value *m1_right = IRB->CreateAnd(m1, getConstant(0x0000ffff));
  m1_right = IRB->CreateShl(m1_right, getConstant("16"));

  Value *m2_left = IRB->CreateAnd(m2, getConstant(0xffff0000));
  m2_left = IRB->CreateLShr(m2_left, getConstant("16"));

  Value *m2_right = IRB->CreateAnd(m2, getConstant(0x0000ffff));
  m2_right = IRB->CreateShl(m2_right, getConstant("16"));

  Z_lo = IRB->CreateAdd(Z_lo, m0);
  auto cf = IRB->CreateICmpULT(Z_lo, m0);
  cf = Bool2Int(cf, IRB);
  Z_hi = IRB->CreateAdd(Z_hi, cf);
  Z_hi = IRB->CreateAdd(Z_hi, m1_left);

  Z_lo = IRB->CreateAdd(Z_lo, m1_right);
  cf = IRB->CreateICmpULT(Z_lo, m1_right);
  cf = Bool2Int(cf, IRB);
  Z_hi = IRB->CreateAdd(Z_hi, cf);
  Z_hi = IRB->CreateAdd(Z_hi, m2_left);

  Z_lo = IRB->CreateAdd(Z_lo, m2_right);
  cf = IRB->CreateICmpULT(Z_lo, m2_right);
  cf = Bool2Int(cf, IRB);
  Z_hi = IRB->CreateAdd(Z_hi, cf);
  Z_hi = IRB->CreateAdd(Z_hi, m3);

  SDNode *node = getFirstOutput(N);
  if (node != NULL) {
    saveNodeValue(N, Z_lo);
    visit(node, IRB);
  }

  saveNodeValue(N, Z_hi);
}

