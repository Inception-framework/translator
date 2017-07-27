#ifndef STORE_LIFTER_H
#define STORE_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class StoreInfo {
 public:
  Type* Ty;

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

    if (next >= iRn_max) return -1;
    return next++;
  };

  StoreInfo(int32_t _n, int32_t _d, int32_t _o, Type* _Ty = NULL,
            int32_t _n_max = -1)
      : Ty(_Ty), iOffset(_o), iRn(_n), iRd(_d), iRn_max(_n_max) {}
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

  // HANDLER_LOAD(tPUSH)
  //
  // HANDLER_LOAD(t2LDMIA_UPD)
  // HANDLER_LOAD(t2LDMIA)
  //
  // HANDLER_LOAD(tSTRr)
  // HANDLER_LOAD(tSTRi)
  // HANDLER_LOAD(t2STRi12)
  // HANDLER_LOAD(t2STRi8)
  // HANDLER_LOAD(t2STR_POST)
  // HANDLER_LOAD(t2STR_PRE)
  // HANDLER_LOAD(t2LDMDB)
  // HANDLER_LOAD(t2STRs)
  //
  // HANDLER_LOAD(tSTRBi)
  // HANDLER_LOAD(t2STRBi8)
  // HANDLER_LOAD(t2STRBi12)
  // HANDLER_LOAD(t2STRB_PRE)
  // HANDLER_LOAD(t2STRB_POST)
  // HANDLER_LOAD(t2LDMDB_UPD)
  // HANDLER_LOAD(t2STRBs)
  //
  // HANDLER_LOAD(t2STRDi8)
  // HANDLER_LOAD(t2STRD_PRE)
  // HANDLER_LOAD(t2STRD_POST)
  //
  // HANDLER_LOAD(tSTRHi)
  // HANDLER_LOAD(t2STRHi12)
  // HANDLER_LOAD(t2STRHi8)
  // HANDLER_LOAD(t2STRH_PRE)
  // HANDLER_LOAD(t2STRH_POST)
  // HANDLER_LOAD(t2STRHs)
};

#endif
