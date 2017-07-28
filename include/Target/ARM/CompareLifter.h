#ifndef COMPARE_LIFTER_H
#define COMPARE_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class CompareLifter : public ARMLifter{
  public :

  void registerLifter();

  CompareLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~CompareLifter(){};

 protected:
  void CompareHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void CompareNHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
