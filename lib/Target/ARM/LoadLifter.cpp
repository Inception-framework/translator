#include "Target/ARM/LoadLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void LoadLifter::registerLifter() {
#define REGISTER_LOAD_OPCODE(opcode, handler)                                 \
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)ARM::opcode, \
                      (LifterHandler)&LoadLifter::handler##Handler);

  REGISTER_LOAD_OPCODE(tPOP, tPOP)

  REGISTER_LOAD_OPCODE(t2LDMIA_UPD, t2LDMIA_UPD)
  REGISTER_LOAD_OPCODE(t2LDMIA, t2LDMIA)

  REGISTER_LOAD_OPCODE(tLDRr, tLDRr)
  REGISTER_LOAD_OPCODE(t2LDRDi8, t2LDRDi8)
  REGISTER_LOAD_OPCODE(t2LDMDB_UPD, t2LDMDB_UPD)
  REGISTER_LOAD_OPCODE(t2LDMDB, t2LDMDB)

  REGISTER_LOAD_OPCODE(tLDRi, tLDRi)
  REGISTER_LOAD_OPCODE(t2LDRi8, t2LDRi8)
  REGISTER_LOAD_OPCODE(t2LDRi12, t2LDRi12)
  REGISTER_LOAD_OPCODE(t2LDRs, t2LDRs)
  REGISTER_LOAD_OPCODE(t2LDR_PRE, t2LDR_PRE)
  REGISTER_LOAD_OPCODE(t2LDR_POST, t2LDR_POST)

  REGISTER_LOAD_OPCODE(tLDRBi, tLDRBi)
  REGISTER_LOAD_OPCODE(t2LDRBi8, t2LDRBi8)
  REGISTER_LOAD_OPCODE(t2LDRBi12, t2LDRBi12)
  REGISTER_LOAD_OPCODE(t2LDRB_PRE, t2LDRB_PRE)
  REGISTER_LOAD_OPCODE(t2LDRBs, t2LDRBs)

  REGISTER_LOAD_OPCODE(tLDRHi, tLDRHi)
  REGISTER_LOAD_OPCODE(t2LDRHi12, t2LDRHi12)
  REGISTER_LOAD_OPCODE(t2LDRH_PRE, t2LDRH_PRE)
  REGISTER_LOAD_OPCODE(t2LDRH_POST, t2LDRH_POST)
  REGISTER_LOAD_OPCODE(t2LDRHs, t2LDRHs)
}

void LoadLifter::t2LDRHsHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1, 3);

  Type* Ty = IntegerType::get(alm->Mod->getContext(), 16);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info =
      new LoadInfo(N, false, false, true, layout, true, true, true, Ty, true);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRBsHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1, 3);

  Type* Ty = IntegerType::get(alm->Mod->getContext(), 8);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info =
      new LoadInfo(N, false, false, true, layout, true, true, true, Ty, true);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRsHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1, 3);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info =
      new LoadInfo(N, false, false, true, layout, true, true, false, NULL, true);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRH_POSTHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  Type* Ty = IntegerType::get(alm->Mod->getContext(), 16);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info =
      new LoadInfo(N, false, true, true, layout, true, false, true, Ty);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRH_PREHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  Type* Ty = IntegerType::get(alm->Mod->getContext(), 16);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before, Trunc,
  // Type
  LoadInfo* info =
      new LoadInfo(N, false, true, true, layout, true, true, true, Ty);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRB_PREHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  Type* Ty = IntegerType::get(alm->Mod->getContext(), 8);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before, Trunc,
  // Type
  LoadInfo* info =
      new LoadInfo(N, false, true, true, layout, true, true, true, Ty);

  LifteNode(info, IRB);
}

void LoadLifter::tLDRHiHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  Type* Ty = IntegerType::get(alm->Mod->getContext(), 16);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before, Trunc,
  // Type
  LoadInfo* info =
      new LoadInfo(N, false, false, true, layout, true, true, true, Ty);

  LifteNode(info, IRB);
}

void LoadLifter::tLDRBiHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  Type* Ty = IntegerType::get(alm->Mod->getContext(), 8);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before, Trunc,
  // Type
  LoadInfo* info =
      new LoadInfo(N, false, false, true, layout, true, true, true, Ty);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRi8Handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, false, false, true, layout, true, true);

  LifteNode(info, IRB);
}

void LoadLifter::tLDRiHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, false, false, true, layout, true, true);

  LifteNode(info, IRB);
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

// XXX: FIXED 20/07/2017
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

  if (info->Trunc) {
    StringRef BaseName = getBaseValueName(Res->getName());
    StringRef Name = getIndexedValueName(BaseName);
    Res = IRB->CreateTrunc(Res, info->Ty, Name);

    Type* Ty = IntegerType::get(alm->Mod->getContext(), 32);

    Name = getIndexedValueName(BaseName);
    Res = IRB->CreateZExt(Res, Ty, Name);
  }

  if (info->OutputDst && !info->OutputAddr) alm->VisitMap[info->N] = Res;
  if (!info->OutputDst && info->OutputAddr) alm->VisitMap[info->N] = Addr;
  if (info->OutputDst && info->OutputAddr) {
    uint32_t i = 0;

    for (SDNode::use_iterator I = info->N->use_begin(), E = info->N->use_end();
         I != E; ++I) {
      if (I->getOpcode() == ISD::CopyToReg &&
          i < info->N->getNumOperands() - 1) {
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

  if (info->Shift) {

    // Compute Register Value
    StringRef BaseName = getBaseValueName(Offset->getName());
    StringRef Name = getIndexedValueName(BaseName);

    i = info->Layout->Shift;
    Value* Op = visit(info->N->getOperand(i).getNode(), IRB);

    Addr = IRB->CreateLShr(Offset, Op, Name);
  }

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
