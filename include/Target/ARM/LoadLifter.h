#ifndef LOAD_LIFTER_H
#define LOAD_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

typedef struct LoadInfo {
  int32_t chain;
  int32_t src_begin;
  int32_t src_end;
  int32_t addr;
  int32_t offset;
  LoadInfo(int32_t _chain, int32_t _src_begin, int32_t _src_end, int32_t _addr,
           int32_t _offset)
      : chain(_chain),
        src_begin(_src_begin),
        src_end(_src_end),
        addr(_addr),
        offset(_offset) {}
} LoadInfo;

class LoadLifter : public ARMLifter {
 public:
  llvm::SDNode* select(llvm::SDNode* node);

 private:
  void InvLoadMultiple(llvm::SDNode* N, bool Inc, bool Before, LoadInfo* info);
};

#endif
