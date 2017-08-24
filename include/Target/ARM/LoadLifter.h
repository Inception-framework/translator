#ifndef LOAD_LIFTER_H
#define LOAD_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class LoadInfo2 {
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
    printf("LoadInfo2:\n");
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

  LoadInfo2(int32_t _n, int32_t _d, int32_t _o, int32_t width = 32,
            int32_t _n_max = -1, bool _shifted = false)
      : iOffset(_o),
        iRn(_n),
        next(_n),
        iRd(_d),
        iRn_max(_n_max),
        width(width),
        shifted(_shifted) {}

  LoadInfo2(int32_t _n, int32_t _d, int32_t _o, int32_t width, bool _shifted)
      : iOffset(_o),
        iRn(_n),
        next(_n),
        iRd(_d),
        iRn_max(-1),
        width(width),
        shifted(_shifted) {}

  LoadInfo2(int32_t _n, int32_t _d, int32_t _o, bool _shifted)
      : iOffset(_o),
        iRn(_n),
        next(_n),
        iRd(_d),
        iRn_max(-1),
        width(32),
        shifted(_shifted) {}
};

typedef struct LoadNodeLayout {
  int32_t Dst_start;
  int32_t Dst_end;
  int32_t Offset;
  int32_t Addr;
  int32_t Shift;
  LoadNodeLayout(int32_t _Dst_start, int32_t _Dst_end, int32_t _Offset,
                 int32_t _Addr, int32_t _Shift = -1)
      : Dst_start(_Dst_start),
        Dst_end(_Dst_end),
        Offset(_Offset),
        Addr(_Addr),
        Shift(_Shift) {}
} LoadNodeLayout;

typedef struct LoadInfo {
  llvm::SDNode* N;

  // Loads more than 1 register ?
  bool MultiDest;

  // Output Address ?
  bool OutputAddr;

  // Output Destination Register ?
  bool OutputDst;

  // Node Laout : Can we guess it ?
  LoadNodeLayout* Layout;

  // Do we inc the stack pointer ?
  bool Increment;

  // Update stack pointer before ?
  bool Before;

  // Trunc the result ?
  bool Trunc;

  Type* Ty;

  bool Shift;

  bool Post;

  LoadInfo(llvm::SDNode* _N, bool _MultiDest, bool _OutputAddr, bool _OutputDst,
           LoadNodeLayout* _Layout, bool _Increment, bool _Before,
           bool _Trunc = false, Type* _Ty = NULL, bool _Shift = false,
           bool _Post = false)
      : N(_N),
        Layout(_Layout),
        MultiDest(_MultiDest),
        OutputAddr(_OutputAddr),
        OutputDst(_OutputDst),
        Increment(_Increment),
        Before(_Before),
        Trunc(_Trunc),
        Shift(_Shift),
        Post(_Post),
        Ty(_Ty) {}
} LoadInfo;

class LoadLifter : public ARMLifter {
 public:
  void registerLifter();

  LoadLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  ~LoadLifter(){};

 protected:
  void LifteNode(LoadInfo* info, llvm::IRBuilder<>* IRB);

  llvm::Value* UpdateAddress(LoadInfo* info, llvm::IRBuilder<>* IRB);

  llvm::Value* CreateLoad(LoadInfo* info, IRBuilder<>* IRB, Value* Addr);

  llvm::Value* CreateStore(LoadInfo* info, IRBuilder<>* IRB, Value* Addr,
                           Value* Src);

  llvm::Value* IncPointer(LoadInfo* info, IRBuilder<>* IRB, Value* Addr_int);

  std::map<unsigned, LoadInfo2*> info;

  LoadInfo2* getInfo(unsigned opcode) {
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

#define HANDLER_LOAD2(name) void do##name(llvm::SDNode* N, IRBuilder<>* IRB);

  HANDLER_LOAD2(DPost)
  HANDLER_LOAD2(DPre)
  HANDLER_LOAD2(D)
  HANDLER_LOAD2(Post)
  HANDLER_LOAD2(Pre)
  HANDLER_LOAD2(Signed)
  HANDLER_LOAD2(Multi)
  HANDLER_LOAD2(Pop)
  HANDLER_LOAD2(Common)
  HANDLER_LOAD2(MultiDB)
  HANDLER_LOAD2(PC)
// Declare each handler

#define HANDLER_LOAD(name) \
  void name##Handler(llvm::SDNode* N, IRBuilder<>* IRB);

  HANDLER_LOAD(tPOP)

  HANDLER_LOAD(t2LDMIA_UPD)
  HANDLER_LOAD(t2LDMIA)

  HANDLER_LOAD(tLDRr)
  HANDLER_LOAD(tLDRi)
  HANDLER_LOAD(t2LDRi12)
  HANDLER_LOAD(t2LDRi8)
  HANDLER_LOAD(t2LDR_POST)
  HANDLER_LOAD(t2LDR_PRE)
  HANDLER_LOAD(t2LDMDB)
  HANDLER_LOAD(t2LDRs)

  HANDLER_LOAD(tLDRBi)
  HANDLER_LOAD(t2LDRBi8)
  HANDLER_LOAD(t2LDRBi12)
  HANDLER_LOAD(t2LDRB_PRE)
  HANDLER_LOAD(t2LDRB_POST)
  HANDLER_LOAD(t2LDMDB_UPD)
  HANDLER_LOAD(t2LDRBs)

  HANDLER_LOAD(t2LDRDi8)
  HANDLER_LOAD(t2LDRD_PRE)
  HANDLER_LOAD(t2LDRD_POST)

  HANDLER_LOAD(tLDRHi)
  HANDLER_LOAD(t2LDRHi12)
  HANDLER_LOAD(t2LDRHi8)
  HANDLER_LOAD(t2LDRH_PRE)
  HANDLER_LOAD(t2LDRH_POST)
  HANDLER_LOAD(t2LDRHs)

  // HANDLER(t2LDDRBi8)
};

#endif
