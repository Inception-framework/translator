#ifndef ADD_LIFTER_H
#define ADD_LIFTER_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "Target/ARM/ARMLifter.h"

typedef struct ARMADDInfo{
  llvm::Value* Op0;
  llvm::Value* Op1;
  llvm::StringRef Name;
  ARMADDInfo(llvm::Value* _Op0, llvm::Value* _Op1, llvm::StringRef Name):
    Op0(_Op0), Op1(_Op1), Name(Name) {}
}ARMADDInfo;

class AddLifter : public ARMLifter{

  public:

    AddLifter(ARMLifterManager* _alm) : ARMLifter(_alm) {};

    void registerLifter();

    // Declare each handler
    #define HANDLER(name) \
    void name##Handler(llvm::SDNode* N, IRBuilder<>* IRB) { \
      AddHandler(N, IRB); \
    };

    // HANDLER(tADDrr);
    // HANDLER(tADDhirr)
    // HANDLER(tADDrSPi)
    // HANDLER(tADDspi)
    // HANDLER(tADDi8)
    // HANDLER(tADDframe)
    // HANDLER(tADDi3)
    // HANDLER(tADDrSP)
    // HANDLER(tADDspr)
    // HANDLER(t2ADDSri)
    // HANDLER(t2ADDSrr)
    // HANDLER(t2ADDSrs)
    // HANDLER(t2ADDri)
    // HANDLER(t2ADDri12)
    // HANDLER(t2ADDrr)
    // HANDLER(t2ADDrs)
    // HANDLER(t2ADCri)

  protected:
    void AddHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

    void AdcHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB);

    ARMADDInfo* RetrieveGraphInformation(llvm::SDNode *N, llvm::IRBuilder<>* IRB);
};

#endif
