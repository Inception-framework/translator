#ifndef FLAGS_LIFTER_H
#define FLAGS_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class FlagsLifter : public ARMLifter {
 protected:
  ARMLifter *Lifter;

 public:
  FlagsLifter(ARMLifterManager *_alm) : ARMLifter(_alm){};

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  // TODO : Lifter should retain their name !
  static bool classof(const ARMLifter *From) { return true; }

  void registerLifter(){};

  void WriteNFAdd(IRBuilder<> *IRB, llvm::Value *res);

  void WriteCFShiftR(IRBuilder<> *IRB, llvm::Value *shift_min_1);

  void WriteCFShiftL(IRBuilder<> *IRB, llvm::Value *shift_min_1,
                     llvm::Value *isShift);

  void WriteZF(IRBuilder<> *IRB, llvm::Value *w);

  void WriteAF2(IRBuilder<> *IRB, llvm::Value *r, llvm::Value *lhs,
                llvm::Value *rhs);

  void WriteCFAdc(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *argL);

  void WriteCFAdd(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *argL);

  void WriteCFSub(IRBuilder<> *IRB, llvm::Value *argL, llvm::Value *argR);

  void WriteCFconstant(IRBuilder<> *IRB, uint32_t constant);

  void WriteVFSub(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *lhs,
                  llvm::Value *rhs);

  void WriteVFAdd(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *lhs,
                  llvm::Value *rhs);

  void WriteVFAdc(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *lhs,
                  llvm::Value *rhs);

  void WriteNF(IRBuilder<> *IRB, llvm::Value *written);
};

#endif
