#include "Target/ARM/AddLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"

#include "Utils/Builder.h"

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
                      (LifterHandler)&AddLifter::AdcHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::t2ADCrr,
                      (LifterHandler)&AddLifter::AdcHandler);
  alm->registerLifter(this, std::string("AddLifter"), (unsigned)ARM::tADC,
                      (LifterHandler)&AddLifter::AdcHandler);
}

void AddLifter::AdcHandler(SDNode *N, IRBuilder<> *IRB) {
  auto cf = ReadReg(Reg("CF"), IRB);

  AddHandler(N, IRB);  // Si opérande à la même position
  Value *Res_add = getSavedValue(N);

  // then
  Value *Res_adc = IRB->CreateAdd(Res_add, cf);

  bool S = IsSetFlags(N);
  if (S) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    // Compute NF.
    flags->WriteNF(IRB, Res_adc);
    // Compute ZF.
    flags->WriteZF(IRB, Res_adc);
    // Ccompute VF.
    flags->WriteVFAdc(IRB, Res_adc, Res_add, cf);
    // Compute CF.
    flags->WriteCFAdc(IRB, Res_adc, Res_add);
  }

  saveNodeValue(N, Res_adc);
}

void AddLifter::AddHandler(SDNode *N, IRBuilder<> *IRB) {
  ARMADDInfo *info = RetrieveGraphInformation(N, IRB);

  Instruction *Res =
      dyn_cast<Instruction>(IRB->CreateAdd(info->Op0, info->Op1));

  Res->setDebugLoc(N->getDebugLoc());

  saveNodeValue(N, Res);

  if (info->S) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    ////Compute NF
    // flags->WriteNFAdd(IRB, Res);
    // Compute NF.
    flags->WriteNF(IRB, Res);
    // Compute ZF.
    flags->WriteZF(IRB, Res);
    // Ccompute VF.
    flags->WriteVFAdd(IRB, Res, info->Op0, info->Op1);
    // Compute CF.
    flags->WriteCFAdd(IRB, Res, info->Op0);
  }
}

ARMADDInfo *AddLifter::RetrieveGraphInformation(SDNode *N, IRBuilder<> *IRB) {
  Value *Op0 = visit(N->getOperand(0).getNode(), IRB);
  Value *Op1 = visit(N->getOperand(1).getNode(), IRB);
  bool S = IsSetFlags(N);
  ARMADDInfo *info = new ARMADDInfo(Op0, Op1, S);

  return info;
}
