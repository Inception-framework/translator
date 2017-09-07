#include "Target/ARM/CompareLifter.h"
#include "Target/ARM/FlagsLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void CompareLifter::registerLifter() {
  // CMP
  alm->registerLifter(this, std::string("CompareLifter"),
                      (unsigned)ARM::t2CMPri,
                      (LifterHandler)&CompareLifter::CompareHandler);
  alm->registerLifter(this, std::string("CompareLifter"),
                      (unsigned)ARM::t2CMPrr,
                      (LifterHandler)&CompareLifter::CompareHandler);
  alm->registerLifter(this, std::string("CompareLifter"), (unsigned)ARM::tCMPr,
                      (LifterHandler)&CompareLifter::CompareHandler);
  alm->registerLifter(this, std::string("CompareLifter"), (unsigned)ARM::tCMPi8,
                      (LifterHandler)&CompareLifter::CompareHandler);

  alm->registerLifter(this, std::string("CompareLifter"),
                      (unsigned)ARM::tCMPhir,
                      (LifterHandler)&CompareLifter::CompareHandler);

  // CMN
  alm->registerLifter(this, std::string("CompareLifter"), (unsigned)ARM::tCMNz,
                      (LifterHandler)&CompareLifter::CompareNHandler);
  alm->registerLifter(this, std::string("CompareLifter"),
                      (unsigned)ARM::t2CMNzrr,
                      (LifterHandler)&CompareLifter::CompareNHandler);
  alm->registerLifter(this, std::string("CompareLifter"),
                      (unsigned)ARM::t2CMNri,
                      (LifterHandler)&CompareLifter::CompareNHandler);
}

// CMP
void CompareLifter::CompareHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  ARMCMPInfo *info = RetrieveGraphInformation(N, IRB);

  // subtraction
  Value *onescompl = IRB->CreateNot(info->Op1);
  Value *Res_add = IRB->CreateAdd(info->Op0, onescompl);
  Value *Res = IRB->CreateAdd(Res_add, getConstant("1"));

  // Res->setDebugLoc(N->getDebugLoc());

  // Write the flag updates.
  // Compute AF.
  FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));
  // Compute NF.
  flags->WriteNF(IRB, Res);
  // Compute ZF.
  flags->WriteZF(IRB, Res);
  // Ccompute VF.
  flags->WriteVFAdd(IRB, Res_add, info->Op0, onescompl);
  flags->WriteVFAdc(IRB, Res, Res_add, getConstant("1"));
  // Compute CF.
  flags->WriteCFAdd(IRB, Res_add, info->Op0);
  flags->WriteCFAdc(IRB, Res, Res_add);

  // Dummy CPSR, not used, Flags are used instead if necessary
  Value *dummyCPSR = getConstant("0");

  saveNodeValue(N, dummyCPSR);
}

// CMN
void CompareLifter::CompareNHandler(llvm::SDNode *N, llvm::IRBuilder<> *IRB) {
  ARMCMPInfo *info = RetrieveGraphInformation(N, IRB);

  Instruction *Res =
      dyn_cast<Instruction>(IRB->CreateAdd(info->Op0, info->Op1));

  Res->setDebugLoc(N->getDebugLoc());

  // Write the flag updates.
  // Compute AF.
  FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));
  // Compute NF.
  flags->WriteNF(IRB, Res);
  // Compute ZF.
  flags->WriteZF(IRB, Res);
  // Ccompute VF.
  flags->WriteVFAdd(IRB, Res, info->Op0, info->Op1);
  // Compute CF.
  flags->WriteCFAdd(IRB, Res, info->Op0);

  // Dummy CPSR, not used, Flags are used instead if necessary
  Value *dummyCPSR = getConstant("0");

  saveNodeValue(N, dummyCPSR);
}

ARMCMPInfo *CompareLifter::RetrieveGraphInformation(SDNode *N,
                                                    IRBuilder<> *IRB) {
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);

  ARMCMPInfo *info = new ARMCMPInfo(Op0, Op1);

  return info;
}
