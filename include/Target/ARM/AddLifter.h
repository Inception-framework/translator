#ifndef ADD_LIFTER_H
#define ADD_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class AddLifter : public ARMLifter{
  public :
    llvm::SDNode* select(llvm::SDNode* node);
};

#endif
