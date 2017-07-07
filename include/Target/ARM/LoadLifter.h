#ifndef LOAD_LIFTER_H
#define LOAD_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class LoadLifter : public ARMLifter{
  public :
    llvm::SDNode* select(llvm::SDNode* node);
};

#endif
