#ifndef LOGICAL_LIFTER_H
#define LOGICAL_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class LogicalLifter : public ARMLifter {
 public:

  void registerLifter();

  LogicalLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  ~LogicalLifter(){};

 protected:
  void TstHandlerRI(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void TstHandlerRR(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void TstHandlerRS(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void BitwiseHandlerRI(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void BitwiseHandlerRR(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void BitwiseHandlerRS(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
