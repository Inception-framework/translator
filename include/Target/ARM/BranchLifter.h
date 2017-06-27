#ifndef BRANCH_LIFTER_H
#define BRANCH_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class BranchLifter : public ARMLifter{
  public :
    llvm::SDNode* select(llvm::SDNode* node);
};

#endif
