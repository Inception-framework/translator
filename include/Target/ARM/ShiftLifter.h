#ifndef SHIFT_LIFTER_H
#define SHIFT_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"
#include "llvm/IR/IRBuilder.h"

typedef struct ARMSHIFTInfo {
  llvm::Value* Op0;
  llvm::Value* Op1;
  bool S;
  ARMSHIFTInfo(llvm::Value* _Op0, llvm::Value* _Op1, bool _S)
      : Op0(_Op0), Op1(_Op1) {
    S = _S;
  }
} ARMSHIFTInfo;

class ARMLifterManager;

class ShiftLifter : public ARMLifter{
  public :

  void registerLifter();

  ShiftLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~ShiftLifter(){};

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  // TODO : Lifter should retain their name !
  // XXX: Absolutely bad way !
  static bool classof(const ARMLifter* From) { return true; }

  void ShiftHandlerShiftOp(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

 protected:
  void ShiftHandlerLSL(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void ShiftHandlerLSR(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void ShiftHandlerASR(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void ShiftHandlerROR(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void ShiftHandlerRRX(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  ARMSHIFTInfo* RetrieveGraphInformation(llvm::SDNode* N,
                                         llvm::IRBuilder<>* IRB);
};

#endif
