#ifndef SVCALL_LIFTER_H
#define SVCALL_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"
#include "llvm/IR/IRBuilder.h"

class ARMLifterManager;

class SVCallLifter : public ARMLifter{
public:

  void registerLifter();

  SVCallLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~SVCallLifter(){};

protected:
  void SVCallHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
