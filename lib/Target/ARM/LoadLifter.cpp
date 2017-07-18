#include "Target/ARM/LoadLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void LoadLifter::registerLifter() {
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::tPOP,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::tLDRspi,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"),
                      (unsigned)ARM::t2LDR_POST,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"),
                      (unsigned)ARM::t2LDMIA_UPD,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMIA_UPD,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMIB_UPD,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMDA_UPD,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMDB_UPD,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMIA,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMIB,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMDA,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMDB,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::t2LDRi12,
                      (LifterHandler)&LoadLifter::LoadHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::t2LDR_PRE,
                      (LifterHandler)&LoadLifter::LoadHandler);
}

void LoadLifter::LoadHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {}
