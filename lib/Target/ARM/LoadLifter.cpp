#include "Target/ARM/LoadLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void LoadLifter::registerLifter() {

#define REGISTER(opcode, handler)                                             \
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::opcode, \
                      (LifterHandler)&LoadLifter::handler##Handler);

  REGISTER(tPOP, tPOP)
  REGISTER(t2LDR_POST, t2LDR_POST)
  REGISTER(t2LDMIA_UPD, t2LDMIA_UPD)
  REGISTER(t2LDMIA, t2LDMIA)
  REGISTER(tLDRr, tLDRr)
  REGISTER(t2LDRHi12, t2LDRHi12)
  REGISTER(t2LDRDi8, t2LDRDi8)
  REGISTER(t2LDMDB_UPD, t2LDMDB_UPD)
  REGISTER(t2LDMDB, t2LDMDB)
  REGISTER(t2LDR_PRE, t2LDR_PRE)
  REGISTER(t2LDRBi8, t2LDRBi8)
  REGISTER(t2LDRBi12, t2LDRBi12)
  REGISTER(t2LDRi12, t2LDRi12)
}

void LoadLifter::t2LDRBi12Handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, false, false, true, layout, true, false);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRBi8Handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, false, false, true, layout, true, false);

  LifteNode(info, IRB);
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
  LoadInfo* info = new LoadInfo(N, false, false, true, layout, true, true);

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
  LoadInfo* info = new LoadInfo(N, true, false, true, layout, true, true);

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
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 1, 0);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, false, false, false, layout, true, true);

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

  if (!info->MultiDest && info->OutputDst) {
    // Load value
    Res = CreateLoad(info, IRB, Addr);

    if (!info->Before && layout->Offset != -1) {
      Addr = UpdateAddress(info, IRB);
    }

  } else if (!info->OutputDst && info->MultiDest) {
    for (unsigned i = layout->Dst_start; i < layout->Dst_end; ++i) {
      // Retrieve destination register
      // Value* Op = visit(info->N->getOperand(i).getNode(), IRB);
      SDNode* pred = info->N->getOperand(i).getNode();
      Value* Op = visitRegister(pred->getOperand(1).getNode(), IRB);

      // Load value
      Res = CreateLoad(info, IRB, Addr);

      if (!info->OutputDst) Res = CreateStore(info, IRB, Op, Res);

      // Increment SP
      Addr = IncPointer(info, IRB, Addr_int);
      Addr_int = Addr;
    }
  } else {
    unsigned i = 0;
    for (SDNode::use_iterator I = info->N->use_begin(), E = info->N->use_end();
         I != E; ++I) {
      if (i >= 2) break;

      if (I->getOpcode() == ISD::CopyToReg) {
        SDNode* pred = *I;

        // Load value
        Res = CreateLoad(info, IRB, Addr);

        alm->VisitMap[info->N] = Res;

        visit(pred, IRB);

        Addr = IncPointer(info, IRB, Addr_int);

      }
      i++;
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
