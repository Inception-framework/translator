#ifndef DIV_LIFTER_H
#define DIV_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class DivLifter : public ARMLifter {
 public:
  DivLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  void registerLifter();

 protected:

  void UDIVHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
