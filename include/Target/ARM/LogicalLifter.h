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
  void TeqHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void AndHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void EorHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void OorHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void OrnHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void BicHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
