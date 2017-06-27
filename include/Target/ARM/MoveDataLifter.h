#ifndef MOVE_DATA_LIFTER_H
#define MOVE_DATA_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class MoveDataLifter : public ARMLifter{
  public :
    llvm::SDNode* select(llvm::SDNode* node);
};

#endif
