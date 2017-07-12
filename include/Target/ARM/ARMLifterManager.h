#ifndef ARM_LIFTER_MANAGER_H
#define ARM_LIFTER_MANAGER_H

#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"

#include <iostream>
#include <vector>

namespace fracture {
  class Decompiler;
}

class ARMLifterManager {
public:
  static ARMLifter *resolve(std::string domain);

  static void FixChainOp(llvm::SDNode *N);

  static llvm::SelectionDAG *DAG;

  static fracture::Decompiler *Dec;

private:
  static std::map<std::string, ARMLifter *> managers;
};

#endif
