
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

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetRegisterInfo.h"

#include "CodeInv/Decompiler.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Utils/Builder.h"
#include "Utils/IContext.h"

#ifndef ARM_LIFTER_H
#define ARM_LIFTER_H

#include "ARMLifterManager.h"

class ARMLifterManager;
class ARMLifter;

#ifndef LIFTER_HANDLER_H
#define LIFTER_HANDLER_H
typedef void (ARMLifter::*LifterHandler)(llvm::SDNode* N,
                                         llvm::IRBuilder<>* IRB);
#endif

namespace fracture {
class Decompiler;
}

class ARMLifter {
 public:
  virtual void registerLifter() = 0;

  ARMLifter(ARMLifterManager* _alm) : alm(_alm){};

 protected:
  llvm::Value* visit(const llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  #define HANDLER(name) \
    void name##Handler(llvm::SDNode* N, IRBuilder<>* IRB) { AddHandler(N, IRB); };

  // Store handler for each supported opcode
  std::map<unsigned, LifterHandler> solver;

  llvm::Value* visitRegister(const llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  llvm::Value* visitCopyFromReg(const llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  llvm::Value* visitCopyToReg(const llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  llvm::Value* visitConstant(const llvm::SDNode* N);

  ARMLifterManager* alm;

};

#endif
