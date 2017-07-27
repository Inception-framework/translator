#ifndef SUB_LIFTER_H
#define SUB_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/ARMLifter.h"
#include "llvm/IR/IRBuilder.h"

typedef struct ARMSHIFTInfo {
  llvm::Value* Op0;
  llvm::Value* Op1;
  ARMSHIFTInfo(llvm::Value* _Op0, llvm::Value* _Op1)
      : Op0(_Op0), Op1(_Op1) {}
} ARMSHIFTInfo;

class ARMLifterManager;

class ShiftLifter : public ARMLifter{
  public :

  void registerLifter();

  ShiftLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

  ~ShiftLifter(){};

  //// Declare each handler
  //#define HANDLER(name)                                     \
//  void name##Handler(llvm::SDNode* N, IRBuilder<>* IRB) { \
//    ShiftHandler(N, IRB);                                 \
//  };
  //
  //  HANDLER(t2LSLri)
  //
 protected:
  void ShiftHandlerLSL(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void ShiftHandlerLSR(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void ShiftHandlerASR(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
  void ShiftHandlerROR(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  ARMSHIFTInfo* RetrieveGraphInformation(llvm::SDNode* N,
                                         llvm::IRBuilder<>* IRB);
};

#endif
