
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

#include "Builder.h"
#include "IContext.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

class ABIAdapter {
 public:
  ABIAdapter();

  ~ABIAdapter();

  Value* getNext(IRBuilder<>* IRB);

  Value* Higher(Function* Func, IRBuilder<>* IRB);

  Value* Lower(Value* inst, IRBuilder<>* IRB);

  Value* Higher(Type* Ty, IRBuilder<>* IRB);

 private:
  unsigned index;

  std::vector<std::string> destinations;

  Value* sp;

  Value* HigherCollections(Type* Ty, unsigned size, IRBuilder<>* IRB);

  Value* HigherInteger(Type* Ty, IRBuilder<>* IRB);

  Value* HigherFloat(Type* type, IRBuilder<>* IRB);

  Value* HigherPointer(Type* Ty, IRBuilder<>* IRB);

  Value* LowerCollections(Value* collection, unsigned size, IRBuilder<>* IRB);

  Value* LowerInteger(Value* integer, IRBuilder<>* IRB);

  Value* LowerPointer(Value* pointer, IRBuilder<>* IRB);
};
