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

#include "Target/ARM/ARMLifter.h"
#include "Target/ARM/ARMLifterManager.h"

#include "Target/ARM/ARMISD.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#define DEBUG_TYPE "ARMLifter.cpp"

using namespace llvm;

Value* ARMLifter::visit(const SDNode* N, IRBuilder<>* IRB) {
  if (IContext::VisitMap.find(N) != IContext::VisitMap.end()) {
    return IContext::VisitMap[N];
  }

  IRB->SetCurrentDebugLocation(N->getDebugLoc());

  switch (N->getOpcode()) {
    default: {
      inception_error(
          "ARMLifter::visit - Every visit should be implemented..."
          "When processing opcode H%08x (D%d)",
          N->getOpcode(), N->getOpcode());
      return NULL;
    }
    case ISD::EntryToken:
      return NULL;
    case ISD::UNDEF:
      return NULL;
    case ISD::CopyFromReg:
      return visitCopyFromReg(N, IRB);
    case ISD::CopyToReg:
      return visitCopyToReg(N, IRB);
    case ISD::Constant:
      return visitConstant(N);
  }
  return NULL;
}

Value* ARMLifter::visitRegister(const SDNode* N, IRBuilder<>* IRB) {
  const RegisterSDNode* R = dyn_cast<RegisterSDNode>(N);
  if (R == NULL) {
    // WARNING("ARMLifter", "VisitRegister -> Not a RegisterSDNode.");
    return NULL;
  }

  Value* Reg = getSavedValue(N);
  if (Reg == NULL) {
    std::string RegName = getRegisterSDNodeName(R);

    if (RegName.find("noreg") != std::string::npos) return NULL;

    Type* Ty = R->getValueType(0).getTypeForEVT(IContext::getContextRef());

    Reg = getModule()->getGlobalVariable(RegName);
    if (Reg == NULL) {
      Constant* Initializer = Constant::getNullValue(Ty);

      GlobalVariable* gvar_ptr =
          new GlobalVariable(*getModule(),  // Module
                             Ty,            // Type
                             false,         // isConstant
                             GlobalValue::CommonLinkage, Initializer, RegName);

      gvar_ptr->setAlignment(4);
      Reg = gvar_ptr;
    }
    saveNodeValue(N, Reg);
  }
  return Reg;
}

Value* ARMLifter::visitCopyFromReg(const SDNode* N, IRBuilder<>* IRB) {
  Value* Res = NULL;
  uint64_t address = 4;

  if (!N->hasAnyUseOfValue(0)) {
    // WARNING("ARMLifter","visitCopyFromReg -> obsolete node");
    return NULL;
  }

  Value* RegVal = visitRegister(N->getOperand(1).getNode(), IRB);
  if (RegVal == NULL) {
    return NULL;
  }

  if (IsPC(N->getOperand(1).getNode())) {
    address += alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());

    Res = getInteger(address);

  } else {
    Res = IRB->CreateLoad(RegVal);
  }

  saveNodeValue(N, Res);
  return Res;
}

Value* ARMLifter::visitCopyToReg(const SDNode* N, IRBuilder<>* IRB) {
  Value* RegVal = visitRegister(N->getOperand(1).getNode(), IRB);
  Value* V = visit(N->getOperand(2).getNode(), IRB);

  if (V == NULL || RegVal == NULL) {
    // WARNING("ARMLifter","visitCopyToReg -> value or register NULL ");
    return NULL;
  }

  if (IsITSTATE(N->getOperand(1).getNode())) {
    alm->Dec->it_start = true;
  }

  if (!RegVal->getType()->isPointerTy()) {
    RegVal = IRB->CreateIntToPtr(RegVal, RegVal->getType()->getPointerTo());
  }

  Instruction* Res = IRB->CreateStore(V, RegVal);
  saveNodeValue(N, Res);
  Res->setDebugLoc(N->getDebugLoc());
  return Res;
}

Value* ARMLifter::visitConstant(const SDNode* N) {
  if (const ConstantSDNode* CSDN = dyn_cast<ConstantSDNode>(N)) {
    Value* Res = Constant::getIntegerValue(
        N->getValueType(0).getTypeForEVT(IContext::getContextRef()),
        CSDN->getAPIntValue());
    saveNodeValue(N, Res);
    return Res;
  } else {
    llvm::errs() << "Could not convert ISD::Constant to integer!\n";
  }
  return NULL;
}
