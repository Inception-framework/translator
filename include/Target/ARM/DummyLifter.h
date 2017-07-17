#ifndef DUMMY_LIFTER_H
#define DUMMY_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class DummyLifter : public ARMLifter{

  public:

    DummyLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

    void registerLifter();

    void handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
