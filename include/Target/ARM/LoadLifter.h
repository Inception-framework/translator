#ifndef LOAD_LIFTER_H
#define LOAD_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

typedef struct LoadInfo {
  llvm::SDNode* N;
  int32_t chain;
  int32_t src_begin;
  int32_t src_end;
  int32_t addr;
  int32_t offset;

  llvm::SDValue Chain;
  llvm::SDValue Offset;
  llvm::SDValue Ptr;
  llvm::SDValue ResNode;
  llvm::SDValue PrevPtr;
  bool Increment;
  bool Before;
  uint16_t MathOpc;

  LoadInfo(llvm::SDNode* _N, int32_t _chain, int32_t _src_begin,
           int32_t _src_end, int32_t _addr, int32_t _offset, bool _inc,
           bool _before)
      : N(_N),
        chain(_chain),
        src_begin(_src_begin),
        src_end(_src_end),
        addr(_addr),
        offset(_offset),
        Increment(_inc),
        Before(_before) {}
} LoadInfo;

class LoadLifter : public ARMLifter {
 public:
  llvm::SDNode* select(llvm::SDNode* node);

 private:
  void InvLoadMultiple(LoadInfo* info);

  void update(llvm::SDValue ResNode);

  llvm::SDValue update_pointer(llvm::SDNode* N, uint16_t MathOpc,
                               llvm::SDValue Ptr, llvm::SDValue Offset,
                               bool UpdateRef);

  void clean_graph(llvm::SDNode* N, llvm::SDValue Chain, llvm::SDValue Ptr);

  llvm::SDValue create_load(llvm::SDNode* N, llvm::SDValue Val,
                            llvm::SDValue Chain, llvm::SDValue Ptr);

  llvm::SDValue create_ret(llvm::SDNode* N, llvm::SDValue Chain, llvm::SDValue Ptr);

  bool is_pc_affected(llvm::SDValue Val);
};

#endif
