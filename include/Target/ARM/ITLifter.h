#ifndef IT_LIFTER_H
#define IT_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class ITLifter : public ARMLifter {
 public:
  ITLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  void registerLifter();

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  // TODO : Lifter should retain their name !
  // XXX: Absolutely bad way !
  static bool classof(const ARMLifter* From) { return true; }

  // protected:
  void ITHandler(const llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void DummyITHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
};

#endif
