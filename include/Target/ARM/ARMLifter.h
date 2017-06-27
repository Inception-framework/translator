#ifndef ARM_LIFTER_H
#define ARM_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

class ARMLifter {
public:
  virtual llvm::SDNode *select(llvm::SDNode *node) = 0;

protected:
  static llvm::SDNode *generic2OP(llvm::SDNode *node, uint16_t opcode);
};

#endif
