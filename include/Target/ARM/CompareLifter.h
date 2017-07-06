#ifndef COMPARE_LIFTER_H
#define COMPARE_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class CompareLifter : public ARMLifter {
  public :
    llvm::SDNode* select(llvm::SDNode* node);
};

#endif
