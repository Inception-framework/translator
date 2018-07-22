
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

#ifndef ARM_LIFTER_MANAGER_H
#define ARM_LIFTER_MANAGER_H

#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "CodeInv/Decompiler.h"
#include "Target/ARM/ARMLifter.h"

#include <iostream>
#include <map>
#include <string>
#include <tuple>

namespace fracture {
class Decompiler;
}

class ARMLifter;

#ifndef LIFTER_HANDLER_H
#define LIFTER_HANDLER_H
typedef void (ARMLifter::*LifterHandler)(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
#endif

typedef struct LifterSolver {
  LifterHandler handler;

  ARMLifter* lifter;

  std::string name;

  LifterSolver(ARMLifter* _lifter, std::string _name, LifterHandler _handler)
      : handler(_handler), lifter(_lifter), name(_name){};
} LifterSolver;

class ARMLifterManager {
 public:
  ~ARMLifterManager();

  ARMLifterManager();

  // Return the coresponding architecture dependent Lifter for the specified
  // opcode
  LifterSolver* resolve(unsigned opcode);

  void registerLifter(ARMLifter* lifter, std::string name, unsigned opcode, LifterHandler handler);

  void registerAll();

  ARMLifter* resolve(StringRef name);

 private:
  std::map<unsigned, LifterSolver*> solver;

  std::map<std::string, ARMLifter*> lifters;

public:
  fracture::Decompiler* Dec;

};

#endif
