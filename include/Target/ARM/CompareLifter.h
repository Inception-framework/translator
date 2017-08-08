#ifndef COMPARE_LIFTER_H
#define COMPARE_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

typedef struct ARMCMPInfo {
  llvm::Value* Op0;
  llvm::Value* Op1;
  ARMCMPInfo(llvm::Value* _Op0, llvm::Value* _Op1) : Op0(_Op0), Op1(_Op1) {}
} ARMCMPInfo;

class ARMLifterManager;

class CompareLifter : public ARMLifter{
  public :

  void registerLifter();

  CompareLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~CompareLifter(){};

 protected:
  void CompareHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void CompareNHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  ARMCMPInfo* RetrieveGraphInformation(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
