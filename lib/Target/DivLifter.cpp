
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

#include "Target/ARM/DivLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Utils/Builder.h"

using namespace llvm;
using namespace fracture;

void DivLifter::registerLifter() {
  alm->registerLifter(this, std::string("DivLifter"), (unsigned)ARM::t2UDIV,
                      (LifterHandler)&DivLifter::UDIVHandler);
}

void DivLifter::UDIVHandler(SDNode *N, IRBuilder<> *IRB) {
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);

  Value *Res = IRB->CreateUDiv(Op0, Op1);

  saveNodeValue(N, Res);
}
