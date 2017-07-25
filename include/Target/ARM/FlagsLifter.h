#ifndef FLAGS_LIFTER_H
#define FLAGS_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class FlagsLifter : public ARMLifter {
 protected:
  ARMLifter *Lifter;

 public:
  FlagsLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  // TODO : Lifter should retain their name !
  static bool classof(const ARMLifter *From) {
    return true;
  }

  void registerLifter() {};

  void WriteAFAddSub(llvm::IRBuilder<> *IRB, llvm::Value *res, llvm::Value *o1,
                     llvm::Value *o2);

};

#endif
