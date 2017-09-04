#ifndef COPROC_LIFTER_H
#define COPROC_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class CoprocLifter : public ARMLifter {
 public:
  CoprocLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  void registerLifter();

 protected:
  void MRSHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  void MSRHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
