
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

#include "IContext.h"
#include "Target/ARM/ARMBaseInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Target/TargetRegisterInfo.h"

#ifndef BUILDER_H
#define BUILDER_H

using namespace llvm;
using namespace fracture;

void initAPI(llvm::Module* _Mod, Decompiler* DEC);

std::string getReg(const SDNode* N);

SDNode* LookUpSDNode(SDNode* N, std::string name);

bool IsCPSR(SDNode* N);

bool IsPC(SDNode* N);

bool IsITSTATE(SDNode* N);

bool IsSigned(SDNode* N);

bool IsSetFlags(SDNode* N);

Value* Bool2Int(Value* v, IRBuilder<>* IRB);

Value* Reg(StringRef name);

Value* Reg(StringRef name, Type* Ty);

Value* getConstant(StringRef value);

Value* getConstant(StringRef value, int Width);

Value* getConstant(uint32_t value);

Value* ReadReg(Value* Rn, IRBuilder<>* IRB, int Width, bool Sign);

Value* ReadReg(Value* Rn, IRBuilder<>* IRB, int Width);

Value* ReadReg(Value* Rn, IRBuilder<>* IRB);

Value* WriteReg(Value* Rn, Value* Rd, IRBuilder<>* IRB, int Width, bool extend);

Value* WriteReg(Value* Rn, Value* Rd, IRBuilder<>* IRB);

Value* WriteReg(Value* Rn, Value* Rd, IRBuilder<>* IRB, int Width);

Value* UpdateRd(Value* Rn, Value* Offset, IRBuilder<>* IRB, bool Increment);

void saveNodeValue(const SDNode* N, Value* Rn);

Value* getSavedValue(const SDNode* N);

llvm::Module* getModule();

Value* getInteger(int value);

std::string getRegisterSDNodeName(const RegisterSDNode* R);

Value* createCondition(int cond, IRBuilder<>* IRB);

Constant* GetVoidFunctionPointer(StringRef function_name);

Constant* GetIntIntFunctionPointer(StringRef function_name);

Constant* GetIntFunctionPointer(StringRef function_name);

void CreateCall(SDNode* N, IRBuilder<>* IRB, uint32_t Tgt);

Value* AdaptCollections(Value* collection, unsigned size, IRBuilder<>* IRB);

Value* AdaptInteger(Value* integer, IRBuilder<>* IRB);

Value* AdaptPointer(Value* pointer, IRBuilder<>* IRB);

Value* Adapt(Value* inst, IRBuilder<>* IRB);

#endif
