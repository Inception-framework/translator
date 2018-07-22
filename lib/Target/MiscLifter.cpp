
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

#include "Target/ARM/MiscLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void MiscLifter::registerLifter() {
  alm->registerLifter(this, std::string("MiscLifter"), (unsigned)ARM::t2RBIT,
                      (LifterHandler)&MiscLifter::RBITHandler);
  alm->registerLifter(this, std::string("MiscLifter"), (unsigned)ARM::t2CLZ,
                      (LifterHandler)&MiscLifter::CLZHandler);
  alm->registerLifter(this, std::string("MiscLifter"), (unsigned)ARM::tREV,
                      (LifterHandler)&MiscLifter::REVHandler);
  alm->registerLifter(this, std::string("MiscLifter"), (unsigned)ARM::t2REV,
                      (LifterHandler)&MiscLifter::REVHandler);
}

void MiscLifter::RBITHandler(llvm::SDNode* N, IRBuilder<>* IRB) {
  Value* Op0 = visit(N->getOperand(0).getNode(), IRB);

  Value* Bit = getConstant("0");
  Value* Res = getConstant("0");
  for (int i = 0; i < 32; i++) {
    Bit = IRB->CreateAnd(Op0, getConstant(1 << i));
    int shift = 31 - 2 * i;
    if (shift >= 0) {
      Bit = IRB->CreateShl(Bit, getConstant(shift));
    } else {
      Bit = IRB->CreateLShr(Bit, getConstant(-shift));
    }
    Res = IRB->CreateOr(Res, Bit);
  }

  saveNodeValue(N, Res);
}

void MiscLifter::REVHandler(llvm::SDNode* N, IRBuilder<>* IRB) {
  Value* Op0 = visit(N->getOperand(0).getNode(), IRB);

  Value* Byte = getConstant("0");
  Value* Res = getConstant("0");
  for (int i = 0; i < 32; i += 8) {
    Byte = IRB->CreateAnd(Op0, getConstant(0xff << i));
    int shift = 24 - 2 * i;
    if (shift >= 0) {
      Byte = IRB->CreateShl(Byte, getConstant(shift));
    } else {
      Byte = IRB->CreateLShr(Byte, getConstant(-shift));
    }
    Res = IRB->CreateOr(Res, Byte);
  }

  saveNodeValue(N, Res);
}

// slightly modified version of:
// http://www.icodeguru.com/Embedded/Hacker%27s-Delight/040.htm
void MiscLifter::CLZHandler(llvm::SDNode* N, IRBuilder<>* IRB) {
  Value* x = visit(N->getOperand(0).getNode(), IRB);

  Value* positive = IRB->CreateNot(x);
  positive = IRB->CreateLShr(positive, getConstant(31));
  positive = IRB->CreateTrunc(positive,
                              IntegerType::get(IContext::getContextRef(), 1));
  positive = IRB->CreateSExt(positive,
                             IntegerType::get(IContext::getContextRef(), 32));

  Value* y = NULL;
  Value* m = NULL;
  Value* n = NULL;

  // y = -(x >> 16);  // If left half of x is 0,
  y = IRB->CreateSub(getConstant("0"), IRB->CreateLShr(x, getConstant(16)));
  // m = (y >> 16) & 16;  // set n = 16. If left half
  m = IRB->CreateAnd(IRB->CreateLShr(y, getConstant(16)), getConstant(16));
  // n = 16 - m;          // is nonzero, set n = 0 and
  n = IRB->CreateSub(getConstant(16), m);
  // x = x >> m;          // shift x right 16.
  x = IRB->CreateLShr(x, m);
  //                     // Now x is of the form 0000xxxx.

  // y = x - 0x100;       // If positions 8-15 are 0,
  y = IRB->CreateSub(x, getConstant(0x100));
  // m = (y >> 16) & 8;   // add 8 to n and shift x left 8.
  m = IRB->CreateAnd(IRB->CreateLShr(y, getConstant(16)), getConstant(8));
  // n = n + m;
  n = IRB->CreateAdd(n, m);
  // x = x << m;
  x = IRB->CreateShl(x, m);

  // y = x - 0x1000;     // If positions 12-15 are 0,
  y = IRB->CreateSub(x, getConstant(0x1000));
  // m = (y >> 16) & 4;  // add 4 to n and shift x left 4.
  m = IRB->CreateAnd(IRB->CreateLShr(y, getConstant(16)), getConstant(4));
  // n = n + m;
  n = IRB->CreateAdd(n, m);
  // x = x << m;
  x = IRB->CreateShl(x, m);

  // y = x - 0x4000;     // If positions 14-15 are 0,
  y = IRB->CreateSub(x, getConstant(0x4000));
  // m = (y >> 16) & 2;  // add 2 to n and shift x left 2.
  m = IRB->CreateAnd(IRB->CreateLShr(y, getConstant(16)), getConstant(2));
  // n = n + m;
  n = IRB->CreateAdd(n, m);
  // x = x << m;
  x = IRB->CreateShl(x, m);

  // y = x >> 14;        // Set y = 0, 1, 2, or 3.
  y = IRB->CreateLShr(x, getConstant(14));
  // m = y & ~(y >> 1);  // Set m = 0, 1, 2, or 2 resp.
  m = IRB->CreateAnd(y, IRB->CreateNot(IRB->CreateLShr(y, getConstant(1))));
  // res = (n + 2 - m) & positive
  Value* Res = IRB->CreateAdd(n, getConstant(2));
  Res = IRB->CreateSub(Res, m);
  Res = IRB->CreateAnd(Res, positive);

  saveNodeValue(N, Res);
}
