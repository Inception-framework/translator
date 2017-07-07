#ifndef STORE_LIFTER_H
#define STORE_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class StoreLifter : public ARMLifter{
  public :
    llvm::SDNode* select(llvm::SDNode* node);
};

#endif
