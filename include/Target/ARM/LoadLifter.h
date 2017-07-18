#ifndef LOAD_LIFTER_H
#define LOAD_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class LoadLifter : public ARMLifter{
  public :

  void registerLifter();

  LoadLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~LoadLifter(){};

protected:
  void LoadHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
