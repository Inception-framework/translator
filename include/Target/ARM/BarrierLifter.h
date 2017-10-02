#ifndef BARRIER_LIFTER_H
#define BARRIER_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class BarrierLifter : public ARMLifter {
 public:
  void registerLifter();

  BarrierLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  ~BarrierLifter(){};

 private:
  void BarrierHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
