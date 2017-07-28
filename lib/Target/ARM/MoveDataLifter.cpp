#include "Target/ARM/MoveDataLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void MoveDataLifter::registerLifter() {
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MOVi16,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MOVi,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::tMOVSr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::tMOVi8,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::tMOVr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MOVr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MOVr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MOVi,
                      (LifterHandler)&MoveDataLifter::MoveHandler);

  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNCCi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNr,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNsi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNsr,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::t2MVNi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
}

void MoveDataLifter::MoveHandler(llvm::SDNode* N, IRBuilder<>* IRB) {
  N->dump();

  Value* Op0 = visit(N->getOperand(0).getNode(), IRB);

  alm->VisitMap[N] = Op0;
}

void MoveDataLifter::MoveNotHandler(llvm::SDNode* N, IRBuilder<>* IRB) {

  llvm::outs() << "MoveNotHandler\n";

  Value* Op0 = visit(N->getOperand(0).getNode(), IRB);
  Op0->dump();

  Value* Res = IRB->CreateNot(Op0);
  Res->dump();

  alm->VisitMap[N] = Res;
}
