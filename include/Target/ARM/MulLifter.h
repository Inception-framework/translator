
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

#ifndef MUL_LIFTER_H
#define MUL_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class MulLifter : public ARMLifter {
 public:
  MulLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  void registerLifter();

 protected:
  // TODO move to Utils
  SDNode* getFirstOutput(llvm::SDNode* N) {
    for (SDNode::use_iterator I = N->use_begin(), E = N->use_end(); I != E;
         I++) {
      SDNode* current = *I;

      if (I->getOpcode() != ISD::CopyToReg) continue;

      SDNode* previous = current->getOperand(0).getNode();
      std::string previousName = getReg(previous);

      // If no reg, we have our root element
      if (previousName.find("noreg") != std::string::npos) return current;
    }
    return NULL;
  }

  void MulHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void UmlalHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
