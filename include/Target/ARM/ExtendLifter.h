#ifndef EXTEND_LIFTER_H
#define EXTEND_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class ExtendLifter : public ARMLifter {
 public:

  void registerLifter();

  ExtendLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  ~ExtendLifter(){};

 protected:
  void ExtendHandlerUXTB(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
