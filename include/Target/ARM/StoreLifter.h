#ifndef STORE_LIFTER_H
#define STORE_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class StoreInfo {
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

    static int32_t next = iRn;

    if (next >= iRn_max) {
      next = iRn;
      return -1;
    }
    return next++;
  };

  StoreInfo(int32_t _n, int32_t _d, int32_t _o, int _Width = 32,
            int32_t _n_max = -1)
      : width(_Width), iOffset(_o), iRn(_n), iRd(_d), iRn_max(_n_max) {}
};

class StoreLifter : public ARMLifter {
 public:
  void registerLifter();

  StoreLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  ~StoreLifter(){};

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
};

#endif
