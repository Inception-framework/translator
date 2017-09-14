#ifndef MUL_LIFTER_H
#define MUL_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class MulLifter : public ARMLifter {
 public:
  MulLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  void registerLifter();

 protected:
  // TODO move to Utils
  SDNode* getFirstOutput(llvm::SDNode* N) {
    for (SDNode::use_iterator I = N->use_begin(), E = N->use_end(); I != E;
         I++) {
      SDNode* current = *I;

      if (I->getOpcode() != ISD::CopyToReg) continue;

      SDNode* previous = current->getOperand(0).getNode();
      std::string previousName = getReg(previous);

      // If no reg, we have our root element
      if (previousName.find("noreg") != std::string::npos) return current;
    }
    return NULL;
  }

  void MulHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void UmlalHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
