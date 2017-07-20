#ifndef LOAD_LIFTER_H
#define LOAD_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

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

  LoadInfo(llvm::SDNode* _N, bool _MultiDest, bool _OutputAddr, bool _OutputDst,
           LoadNodeLayout* _Layout, bool _Increment, bool _Before,
           bool _Trunc = false, Type* _Ty = NULL, bool _Shift=false)
      : N(_N),
        Layout(_Layout),
        MultiDest(_MultiDest),
        OutputAddr(_OutputAddr),
        OutputDst(_OutputDst),
        Increment(_Increment),
        Before(_Before),
        Trunc(_Trunc),
        Shift(_Shift),
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

// Declare each handler
#define HANDLER_LOAD(name) \
  void name##Handler(llvm::SDNode* N, IRBuilder<>* IRB);

  HANDLER_LOAD(tPOP)
  HANDLER_LOAD(t2LDMIA_UPD)
  HANDLER_LOAD(t2LDMIA)
  HANDLER_LOAD(tLDRr)
  HANDLER_LOAD(t2LDRi12)
  HANDLER_LOAD(t2LDRHi12)
  HANDLER_LOAD(t2LDRDi8)
  HANDLER_LOAD(t2LDR_POST)
  HANDLER_LOAD(t2LDMDB_UPD)
  HANDLER_LOAD(t2LDMDB)
  HANDLER_LOAD(t2LDR_PRE)
  HANDLER_LOAD(t2LDRBi8)
  HANDLER_LOAD(t2LDRBi12)
  HANDLER_LOAD(tLDRi)
  HANDLER_LOAD(t2LDRi8)
  HANDLER_LOAD(tLDRBi)
  HANDLER_LOAD(t2LDRB_PRE)
  HANDLER_LOAD(tLDRHi)
  HANDLER_LOAD(t2LDRH_PRE)
  HANDLER_LOAD(t2LDRH_POST)
  HANDLER_LOAD(t2LDRs)
  // HANDLER(t2LDDRBi8)
};

#endif
