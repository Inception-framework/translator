
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

#ifndef LOAD_LIFTER_H
#define LOAD_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class LoadInfo {
 private:
  int32_t next;

 public:
  int32_t iOffset;

  int32_t iRn;
  int32_t iRn_max;

  int32_t iRd;

  int32_t width;

  bool shifted;

  void dump() {
    printf("LoadInfo:\n");
    printf("iRn            -> %d\n", iRn);
    printf("iRd            -> %d\n", iRd);
    printf("iOffset        -> %d\n", iOffset);
    printf("width          -> %d\n", width);
    printf("iRn_max        -> %d\n", iRn_max);
  }

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

  LoadInfo(int32_t _n, int32_t _d, int32_t _o, int32_t width = 32,
           int32_t _n_max = -1, bool _shifted = false)
      : next(_n),
        iOffset(_o),
        iRn(_n),
        iRn_max(_n_max),
        iRd(_d),
        width(width),
        shifted(_shifted) {}

  LoadInfo(int32_t _n, int32_t _d, int32_t _o, int32_t width, bool _shifted)
      : next(_n),
        iOffset(_o),
        iRn(_n),
        iRn_max(-1),
        iRd(_d),
        width(width),
        shifted(_shifted) {}

  LoadInfo(int32_t _n, int32_t _d, int32_t _o, bool _shifted)
      : next(_n),
        iOffset(_o),
        iRn(_n),
        iRn_max(-1),
        iRd(_d),
        width(32),
        shifted(_shifted) {}
};

class LoadLifter : public ARMLifter {
 public:
  void registerLifter();

  LoadLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  ~LoadLifter() { info.clear(); };

 protected:
  void LifteNode(LoadInfo* info, llvm::IRBuilder<>* IRB);

  llvm::Value* UpdateAddress(LoadInfo* info, llvm::IRBuilder<>* IRB);

  llvm::Value* CreateLoad(LoadInfo* info, IRBuilder<>* IRB, Value* Addr);

  llvm::Value* CreateStore(LoadInfo* info, IRBuilder<>* IRB, Value* Addr,
                           Value* Src);

  llvm::Value* IncPointer(LoadInfo* info, IRBuilder<>* IRB, Value* Addr_int);

  std::map<unsigned, LoadInfo*> info;

  LoadInfo* getInfo(unsigned opcode) {
    auto search = info.find(opcode);

    if (search != info.end())
      return search->second;
    else
      return NULL;
  }

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

  SDNode* getNextOutput(llvm::SDNode* N, llvm::SDNode* Prev) {
    for (SDNode::use_iterator I = N->use_begin(), E = N->use_end(); I != E;
         I++) {
      SDNode* current = *I;

      if (I->getOpcode() != ISD::CopyToReg) continue;

      SDNode* previous = current->getOperand(0).getNode();

      // If no reg, we have our root element
      if (previous == Prev) return current;
    }
    return NULL;
  }

#define HANDLER_LOAD(name) void do##name(llvm::SDNode* N, IRBuilder<>* IRB);

  HANDLER_LOAD(DPost)
  HANDLER_LOAD(DPre)
  HANDLER_LOAD(D)
  HANDLER_LOAD(Post)
  HANDLER_LOAD(Pre)
  HANDLER_LOAD(Signed)
  HANDLER_LOAD(Multi)
  HANDLER_LOAD(Pop)
  HANDLER_LOAD(Common)
  HANDLER_LOAD(MultiDB)
  HANDLER_LOAD(PC)
  // Declare each handler
};

#endif
