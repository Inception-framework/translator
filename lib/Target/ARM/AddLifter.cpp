#include "Target/ARM/AddLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"

using namespace llvm;
using namespace fracture;

void AddLifter::registerLifter() {
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDrr,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDhirr,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDrSPi,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDspi,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDi8,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDframe,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDi3,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDrSP,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADDspr,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDSri,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDSrr,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDSrs,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDri,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDri12,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDrr,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADDrs,
                      (LifterHandler)&AddLifter::AddHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADCri,
                      (LifterHandler)&AddLifter::AddHandler);
}

void AddLifter::AddHandler(SDNode *N, IRBuilder<> *IRB) {
  ARMADDInfo *info = RetrieveGraphInformation(N, IRB);

  Instruction *Res =
      dyn_cast<Instruction>(IRB->CreateAdd(info->Op0, info->Op1, info->Name));

  Res->setDebugLoc(N->getDebugLoc());

  Type* Ty = IntegerType::get(alm->Mod->getContext(), 32);

  // Write the flag updates.
  // Compute AF.
  FlagsLifter* flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

  flags->WriteAFAddSub(IRB, Res, info->Op0, info->Op1);
  // Compute SF.
  flags->WriteSF(IRB, Res);
  // Compute ZF.
  flags->WriteZF(IRB, Res);
  // Ccompute OF.
  flags->WriteOFAdd(IRB, Res, info->Op0, info->Op1);
  // Compute PF.
  flags->WritePF(IRB, Res);
  // Compute CF.
  flags->WriteCFAdd(IRB, Res, info->Op0);

  alm->VisitMap[N] = Res;
}

ARMADDInfo *AddLifter::RetrieveGraphInformation(SDNode *N, IRBuilder<> *IRB) {
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);

  StringRef BaseName = getInstructionName(N, IRB);
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op0->getName());
  }

  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op1->getName());
  }

  StringRef Name = getIndexedValueName(BaseName);

  ARMADDInfo *info = new ARMADDInfo(Op0, Op1, Name);

  return info;
}
