#ifndef MOVE_DATA_LIFTER_H
#define MOVE_DATA_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

class ARMLifterManager;

class MoveDataLifter : public ARMLifter {
 public:
  void registerLifter();

  MoveDataLifter(ARMLifterManager* _alm) : ARMLifter(_alm){};

  ~MoveDataLifter(){};

 private:
  void MoveHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  void MoveNotHandler(llvm::SDNode* N, IRBuilder<>* IRB);
};

#endif
