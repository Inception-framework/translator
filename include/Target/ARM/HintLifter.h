#ifndef HINT_LIFTER_H
#define HINT_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class HintLifter : public ARMLifter {
 public:
  void registerLifter();

  HintLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  ~HintLifter(){};

 private:
  void HintHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
