
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

#ifndef INCEPTION_CONTEXT_H
#define INCEPTION_CONTEXT_H

#include "Utils/ErrorHandling.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/Module.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "Target/ARM/ARMLifterManager.h"

using namespace llvm;
using namespace inception;

class ARMLifterManager;

class IContext {
 public:
  static IContext* m_instance;

  static LLVMContext* getContext();

  static LLVMContext& getContextRef();

  static llvm::Module* Mod;

  static const TargetRegisterInfo* RegisterInfo;

  static DenseMap<const SDNode*, Value*> VisitMap;

  static ARMLifterManager *alm;

};

#endif
