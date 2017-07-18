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
//
// // Declare each handler
// #define HANDLER(name)                                     \
//   void name##Handler(llvm::SDNode* N, IRBuilder<>* IRB) { \
//     MoveHandler(N, IRB);                                  \
//   };
//
//   HANDLER(t2MOVi16)
//   HANDLER(t2MOVi)
//   HANDLER(t2MVNi)
//   HANDLER(tMOVSr)
//   HANDLER(tMOVi8)
//   HANDLER(tMOVr)
//   HANDLER(MOVr)
//   HANDLER(MOVi)
};

#endif
