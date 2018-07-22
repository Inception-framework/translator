
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

#ifndef FLAGS_LIFTER_H
#define FLAGS_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class FlagsLifter : public ARMLifter {
 protected:
  ARMLifter *Lifter;

 public:
  FlagsLifter(ARMLifterManager *_alm) : ARMLifter(_alm){};

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  // TODO : Lifter should retain their name !
  static bool classof(const ARMLifter *From) { return true; }

  void registerLifter(){};

  void WriteNFAdd(IRBuilder<> *IRB, llvm::Value *res);

  void WriteCFShiftR(IRBuilder<> *IRB, llvm::Value *shift_min_1);

  void WriteCFShiftL(IRBuilder<> *IRB, llvm::Value *shift_min_1,
                     llvm::Value *isShift);

  void WriteZF(IRBuilder<> *IRB, llvm::Value *w);

  void WriteAF2(IRBuilder<> *IRB, llvm::Value *r, llvm::Value *lhs,
                llvm::Value *rhs);

  void WriteCFAdc(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *argL);

  void WriteCFAdd(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *argL);

  void WriteCFSub(IRBuilder<> *IRB, llvm::Value *argL, llvm::Value *argR);

  void WriteCFconstant(IRBuilder<> *IRB, uint32_t constant);

  void WriteVFSub(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *lhs,
                  llvm::Value *rhs);

  void WriteVFAdd(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *lhs,
                  llvm::Value *rhs);

  void WriteVFAdc(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *lhs,
                  llvm::Value *rhs);

  void WriteNF(IRBuilder<> *IRB, llvm::Value *written);
};

#endif
