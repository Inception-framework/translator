#ifndef ADD_LIFTER_H
#define ADD_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

typedef struct ARMADDInfo {
  llvm::Value* Op0;
  llvm::Value* Op1;
  bool S;
  ARMADDInfo(llvm::Value* _Op0, llvm::Value* _Op1, bool _S)
      : Op0(_Op0), Op1(_Op1) {
    S = _S;
  }
} ARMADDInfo;

class AddLifter : public ARMLifter {
 public:
  AddLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  void registerLifter();

 protected:
  void AddsHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  void AddHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  void AdcHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  ARMADDInfo* RetrieveGraphInformation(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
