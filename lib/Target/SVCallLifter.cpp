
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

#include "Target/ARM/SVCallLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"

#include "Utils/IContext.h"

using namespace llvm;
using namespace fracture;

void SVCallLifter::registerLifter() {
  alm->registerLifter(this, std::string("SVCALL"), (unsigned)ARM::tSVC,
                      (LifterHandler)&SVCallLifter::SVCallHandler);
}

void SVCallLifter::SVCallHandler(SDNode* N, IRBuilder<>* IRB) {
  Value* reg = NULL;

  uint32_t PC = alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());

  std::stringstream stream;
  stream << std::hex << (PC - 2);
  std::string PC_hex(stream.str());

  StringRef reg_name("_SVC_" + PC_hex + "\0");
  reg = Reg(reg_name);
  WriteReg(visit(N->getOperand(1).getNode(), IRB), reg, IRB, 32);

  WriteReg(getConstant(PC), Reg("PC"), IRB, 32);

  Constant* fct_ptr = GetVoidFunctionPointer("inception_sv_call");
  std::vector<Value*> params;

  IRB->CreateCall(fct_ptr, params);
}
