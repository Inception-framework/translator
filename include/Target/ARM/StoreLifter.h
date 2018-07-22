
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

#ifndef STORE_LIFTER_H
#define STORE_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"
class ARMLifterManager;

class StoreInfo {
 private:
  int32_t next;

 public:
  int width;

  int32_t iOffset;

  int32_t iRn;
  int32_t iRn_max;

  int32_t iRd;

  bool hasManyUses() {
    if (iRn_max == -1) return false;
    if (iRn_max == iRn) return false;

    return true;
  }

  int32_t getNext() {
    if (iRn_max == -1) {
      iRn_max = iRn;
      return iRn_max;
    }

    if (next >= iRn_max) {
      next = iRn;
      return -1;
    }
    return next++;
  };

  StoreInfo(int32_t _n, int32_t _d, int32_t _o, int _Width = 32,
            int32_t _n_max = -1)
      : next(_n),
        width(_Width),
        iOffset(_o),
        iRn(_n),
        iRn_max(_n_max),
        iRd(_d){};
};

class StoreLifter : public ARMLifter {
 public:
  void registerLifter();

  StoreLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  ~StoreLifter() { info.clear(); };

  std::map<unsigned, StoreInfo*> info;

  StoreInfo* getInfo(unsigned opcode) {
    auto search = info.find(opcode);

    if (search != info.end())
      return search->second;
    else
      return NULL;
  }

#define HANDLER_STORE(name) void do##name(llvm::SDNode* N, IRBuilder<>* IRB);

  HANDLER_STORE(Post)
  HANDLER_STORE(Pre)
  HANDLER_STORE(Signed)
  HANDLER_STORE(Multi)
  HANDLER_STORE(Push)
  HANDLER_STORE(Common)
  HANDLER_STORE(D)
  HANDLER_STORE(MultiDB)
};

#endif
