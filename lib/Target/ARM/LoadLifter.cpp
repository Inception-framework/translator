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
                      (LifterHandler)&LoadLifter::tPOPHandler);
  // alm->registerLifter(this, std::string("LoadLifter"),
  // (unsigned)ARM::tLDRspi,
  //                     (LifterHandler)&LoadLifter::tLDRspiHandler);
  alm->registerLifter(this, std::string("LoadLifter"),
                      (unsigned)ARM::t2LDR_POST,
                      (LifterHandler)&LoadLifter::t2LDR_POSTHandler);
  alm->registerLifter(this, std::string("LoadLifter"),
                      (unsigned)ARM::t2LDMIA_UPD,
                      (LifterHandler)&LoadLifter::t2LDMIA_UPDHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::t2LDMIA,
                      (LifterHandler)&LoadLifter::t2LDMIAHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::tLDRr,
                      (LifterHandler)&LoadLifter::tLDRrHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::t2LDRHi12,
                      (LifterHandler)&LoadLifter::t2LDRHi12Handler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::t2LDRDi8,
                      (LifterHandler)&LoadLifter::t2LDRDi8Handler);
  alm->registerLifter(this, std::string("LoadLifter"),
                      (unsigned)ARM::t2LDMDB_UPD,
                      (LifterHandler)&LoadLifter::t2LDMDB_UPDHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::t2LDMDB,
                      (LifterHandler)&LoadLifter::t2LDMDBHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::t2LDR_PRE,
                      (LifterHandler)&LoadLifter::t2LDR_PREHandler);

  // alm->registerLifter(this, std::string("LoadLifter"),
  // (unsigned)ARM::LDMIB_UPD,
  //                     (LifterHandler)&LoadLifter::LDMIB_UPDHandler);
  // alm->registerLifter(this, std::string("LoadLifter"),
  // (unsigned)ARM::LDMDA_UPD,
  //                     (LifterHandler)&LoadLifter::LDMDA_UPDHandler);
  // alm->registerLifter(this, std::string("LoadLifter"),
  // (unsigned)ARM::LDMDB_UPD,
  //                     (LifterHandler)&LoadLifter::LDMDB_UPDHandler);
  // alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMIA,
  //                     (LifterHandler)&LoadLifter::LDMIAHandler);
  // alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMIB,
  //                     (LifterHandler)&LoadLifter::LDMIBHandler);
  // alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMDA,
  //                     (LifterHandler)&LoadLifter::LDMDAHandler);
  // alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::LDMDB,
  //                     (LifterHandler)&LoadLifter::LDMDBHandler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::t2LDRi12,
                      (LifterHandler)&LoadLifter::t2LDRi12Handler);
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::t2LDR_PRE,
                      (LifterHandler)&LoadLifter::t2LDR_PREHandler);
}

void LoadLifter::tPOPHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(3, max - 1, -1, max - 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, true, true, false, layout, true, false);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRi12Handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, false, false, true, layout, true, true);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDMIA_UPDHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(4, max - 1, -1, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, true, true, false, layout, true, false);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDMIAHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(4, max - 1, -1, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, true, false, false, layout, true, false);

  LifteNode(info, IRB);
}

void LoadLifter::tLDRrHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, false, false, true, layout, true, false);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRHi12Handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, false, false, true, layout, true, false);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRDi8Handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, false, false, true, layout, true, false);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDR_POSTHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, false, true, true, layout, true, false);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDMDB_UPDHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(4, max - 1, -1, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, true, true, false, layout, true, false);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDMDBHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(4, max - 1, -1, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, true, false, false, layout, true, false);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDR_PREHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, false, true, true, layout, true, true);

  LifteNode(info, IRB);
}

void LoadLifter::LifteNode(LoadInfo* info, llvm::IRBuilder<>* IRB) {
  // Retrieve Address
  Value* SavedAddr;
  Value* Addr;

  LoadNodeLayout* layout = info->Layout;

  SavedAddr = Addr = visit(info->N->getOperand(layout->Addr).getNode(), IRB);
  Value* Res;

  if (info->Before && layout->Offset != -1) {
    Addr = UpdateAddress(info, IRB);
  }

  // Backup Ptr
  Value* Addr_int = Addr;

  if (!info->MultiDest) {
    // Load value
    Res = CreateLoad(info, IRB, Addr);

    if (!info->Before && layout->Offset != -1) {
      Addr = UpdateAddress(info, IRB);
    }

  } else {
    for (unsigned i = layout->Dst_start; i < layout->Dst_end; ++i) {
      // Retrieve destination register
      Value* Op = visit(info->N->getOperand(i).getNode(), IRB);

      // Load value
      Res = CreateLoad(info, IRB, Addr);

      if (!info->OutputDst) Res = CreateStore(info, IRB, Op, Res);

      // Increment SP
      if (info->MultiDest) {
        Addr = IncPointer(info, IRB, Addr_int);
        Addr_int = Addr;
      }
    }
  }

  if (info->OutputDst && !info->OutputAddr) alm->VisitMap[info->N] = Res;
  if (!info->OutputDst && info->OutputAddr) alm->VisitMap[info->N] = Addr;
  if (info->OutputDst && info->OutputAddr) {
    uint32_t i = 0;

    for (SDNode::use_iterator I = info->N->use_begin(), E = info->N->use_end();
         I != E; ++I) {
      if (I->getOpcode() == ISD::CopyToReg) {
        SDNode* pred = *I;

        pred->dump();

        if (i == 1) {
          alm->VisitMap[info->N] = Res;
          visit(pred, IRB);
        }

        if (i == 0) {
          alm->VisitMap[info->N] = Addr;
          visit(pred, IRB);
        }
        i++;
      }
    }
  }
}

llvm::Value* LoadLifter::UpdateAddress(LoadInfo* info, llvm::IRBuilder<>* IRB) {
  uint32_t i;

  // Lift Operands
  i = info->Layout->Offset;
  Value* Offset = visit(info->N->getOperand(i).getNode(), IRB);
  i = info->Layout->Addr;
  Value* Addr = visit(info->N->getOperand(i).getNode(), IRB);

  // Compute Register Value
  StringRef BaseName = getBaseValueName(Addr->getName());
  StringRef Name = getIndexedValueName(BaseName);

  // Add Offset to Address
  Addr = dyn_cast<Instruction>(IRB->CreateAdd(Addr, Offset, Name));
  dyn_cast<Instruction>(Addr)->setDebugLoc(info->N->getDebugLoc());

  return Addr;
}

llvm::Value* LoadLifter::CreateLoad(LoadInfo* info, IRBuilder<>* IRB,
                                    Value* Addr) {
  StringRef BaseName = getBaseValueName(Addr->getName());

  if (!Addr->getType()->isPointerTy()) {
    StringRef Name = getIndexedValueName(BaseName);
    Addr = IRB->CreateIntToPtr(Addr, Addr->getType()->getPointerTo(), Name);
    (dyn_cast<Instruction>(Addr))->setDebugLoc(info->N->getDebugLoc());
  }

  StringRef Name = getIndexedValueName(BaseName);
  Value* load = IRB->CreateLoad(Addr, Name);
  dyn_cast<Instruction>(load)->setDebugLoc(info->N->getDebugLoc());

  return load;
}

llvm::Value* LoadLifter::CreateStore(LoadInfo* info, IRBuilder<>* IRB,
                                     Value* Addr, Value* Src) {
  StringRef BaseName = getBaseValueName(Addr->getName());

  if (!Addr->getType()->isPointerTy()) {
    StringRef Name = getIndexedValueName(BaseName);
    Addr = IRB->CreateIntToPtr(Addr, Addr->getType()->getPointerTo(), Name);
  }

  Instruction* store = IRB->CreateStore(Src, Addr);
  store->setDebugLoc(info->N->getDebugLoc());

  return store;
}

llvm::Value* LoadLifter::IncPointer(LoadInfo* info, IRBuilder<>* IRB,
                                    Value* Addr) {
  StringRef BaseName = getBaseValueName(Addr->getName());

  ConstantInt* const_4 =
      ConstantInt::get(alm->Mod->getContext(), APInt(32, StringRef("4"), 10));

  StringRef Name = getIndexedValueName(BaseName);

  Addr = dyn_cast<Instruction>(IRB->CreateAdd(Addr, const_4, Name));

  dyn_cast<Instruction>(Addr)->setDebugLoc(info->N->getDebugLoc());

  return Addr;
}
