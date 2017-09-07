#ifndef BRANCH_LIFTER_H
#define BRANCH_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class BranchLifter : public ARMLifter{
  public :

  void registerLifter();

  BranchLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~BranchLifter(){};

 protected:
  void BranchHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void BranchHandlerB(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void BranchHandlerBL(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void BranchHandlerBLXr(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void BranchHandlerCB(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void CallHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB, uint32_t Tgt);
};

#endif
