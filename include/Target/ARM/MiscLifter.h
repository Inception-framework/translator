#ifndef MISC_LIFTER_H
#define MISC_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class MiscLifter : public ARMLifter {
 public:
  void registerLifter();

  MiscLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  ~MiscLifter(){};

 private:
  void RBITHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
