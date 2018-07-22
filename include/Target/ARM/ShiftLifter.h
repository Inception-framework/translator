
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

#ifndef SHIFT_LIFTER_H
#define SHIFT_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"
#include "llvm/IR/IRBuilder.h"

typedef struct ARMSHIFTInfo {
  llvm::Value* Op0;
  llvm::Value* Op1;
  bool S;
  ARMSHIFTInfo(llvm::Value* _Op0, llvm::Value* _Op1, bool _S)
      : Op0(_Op0), Op1(_Op1) {
    S = _S;
  }
} ARMSHIFTInfo;

class ARMLifterManager;

class ShiftLifter : public ARMLifter{
  public :

  void registerLifter();

  ShiftLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~ShiftLifter(){};

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  // TODO : Lifter should retain their name !
  // XXX: Absolutely bad way !
  static bool classof(const ARMLifter* From) { return true; }

  void ShiftHandlerShiftOp(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

 protected:
  void ShiftHandlerLSL(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void ShiftHandlerLSR(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void ShiftHandlerASR(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void ShiftHandlerROR(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void ShiftHandlerRRX(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  ARMSHIFTInfo* RetrieveGraphInformation(llvm::SDNode* N,
                                         llvm::IRBuilder<>* IRB);
};

#endif
