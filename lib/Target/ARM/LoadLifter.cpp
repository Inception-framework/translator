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

#define REGISTER_LOAD_OPCODE2(opcode, handler, composition)              \
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)opcode, \
                      (LifterHandler)&LoadLifter::do##handler);          \
  info.insert(std::pair<unsigned, LoadInfo2*>((unsigned)opcode, composition));

  REGISTER_LOAD_OPCODE2(ARM::tPOP, Pop, new LoadInfo2(3, -1, 0))
  // REGISTER_LOAD_OPCODE(tPOP, tPOP)

  // REGISTER_LOAD_OPCODE(, t2LDMIA_UPD)
  REGISTER_LOAD_OPCODE2(ARM::t2LDMIA_UPD, Multi, new LoadInfo2(4, 1, 0))
  // REGISTER_LOAD_OPCODE(t2LDMIA_UPD, t2LDMIA_UPD)
  REGISTER_LOAD_OPCODE2(ARM::t2LDMIA, Multi, new LoadInfo2(4, 1, 0))
  // REGISTER_LOAD_OPCODE(t2LDMIA, t2LDMIA)

  REGISTER_LOAD_OPCODE2(ARM::t2LDMDB_UPD, MultiDB,
                        new LoadInfo2(4, 1, -1, 32, 0))
  // REGISTER_LOAD_OPCODE(t2LDMDB_UPD, t2LDMDB_UPD)

  REGISTER_LOAD_OPCODE2(ARM::t2LDMDB, MultiDB, new LoadInfo2(4, 1, -1, 32, 0))
  // REGISTER_LOAD_OPCODE(t2LDMDB, t2LDMDB)

  REGISTER_LOAD_OPCODE2(ARM::tLDRi, Common, new LoadInfo2(-1, 1, 2))
  REGISTER_LOAD_OPCODE2(ARM::tLDRspi, Common, new LoadInfo2(-1, 1, 2))
  // REGISTER_LOAD_OPCODE(tLDRi, tLDRi)
  REGISTER_LOAD_OPCODE2(ARM::tLDRr, Common, new LoadInfo2(-1, 1, 2))
  // REGISTER_LOAD_OPCODE(tLDRr, tLDRr)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRi8, Common, new LoadInfo2(-1, 1, 2))
  // REGISTER_LOAD_OPCODE(t2LDRi8, t2LDRi8)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRi12, Common, new LoadInfo2(-1, 1, 2))
  // REGISTER_LOAD_OPCODE(t2LDRi12, t2LDRi12)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRs, Signed, new LoadInfo2(1, 1, 2, true))
  // REGISTER_LOAD_OPCODE(t2LDRs, t2LDRs)
  REGISTER_LOAD_OPCODE2(ARM::t2LDR_PRE, Pre, new LoadInfo2(-1, 1, 2))
  // REGISTER_LOAD_OPCODE(t2LDR_PRE, t2LDR_PRE)
  REGISTER_LOAD_OPCODE2(ARM::t2LDR_POST, Post, new LoadInfo2(-1, 1, 2))
  // REGISTER_LOAD_OPCODE(t2LDR_POST, t2LDR_POST)

  REGISTER_LOAD_OPCODE2(ARM::tLDRBi, Common, new LoadInfo2(-1, 1, 2, 8))
  // REGISTER_LOAD_OPCODE(tLDRBi, tLDRBi)
  REGISTER_LOAD_OPCODE2(ARM::tLDRBr, Common, new LoadInfo2(-1, 1, 2, 8))
  // REGISTER_LOAD_OPCODE(tLDRBr, tLDRBi)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRBi8, Common, new LoadInfo2(-1, 1, 2, 8))
  // REGISTER_LOAD_OPCODE(t2LDRBi8, t2LDRBi8)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRBi12, Common, new LoadInfo2(-1, 1, 2, 8))
  // REGISTER_LOAD_OPCODE(t2LDRBi12, t2LDRBi12)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRB_PRE, Pre, new LoadInfo2(-1, 1, 2, 8))
  // REGISTER_LOAD_OPCODE(t2LDRB_PRE, t2LDRB_PRE)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRB_POST, Post, new LoadInfo2(-1, 1, 2, 8))
  // REGISTER_LOAD_OPCODE(t2LDRB_POST, t2LDRB_POST)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRBs, Common, new LoadInfo2(-1, 1, 2, 8, true))
  // REGISTER_LOAD_OPCODE(t2LDRBs, t2LDRBs)

  REGISTER_LOAD_OPCODE2(ARM::tLDRHi, Common, new LoadInfo2(-1, 1, 2, 16))
  // REGISTER_LOAD_OPCODE(tLDRHi, tLDRHi)
  REGISTER_LOAD_OPCODE2(ARM::tLDRHr, Common, new LoadInfo2(-1, 1, 2, 16))
  // REGISTER_LOAD_OPCODE(tLDRHr, tLDRHi)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRHi12, Common, new LoadInfo2(-1, 1, 2, 16))
  // REGISTER_LOAD_OPCODE(t2LDRHi12, t2LDRHi12)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRHi8, Common, new LoadInfo2(-1, 1, 2, 16))
  // REGISTER_LOAD_OPCODE(t2LDRHi8, t2LDRHi8)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRH_PRE, Pre, new LoadInfo2(-1, 1, 2, 16))
  // REGISTER_LOAD_OPCODE(t2LDRH_PRE, t2LDRH_PRE)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRH_POST, Post, new LoadInfo2(-1, 1, 2, 16))
  // REGISTER_LOAD_OPCODE(t2LDRH_POST, t2LDRH_POST)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRHs, Common, new LoadInfo2(-1, 1, 2, 16, true))
  // REGISTER_LOAD_OPCODE(t2LDRHs, t2LDRHs)

  REGISTER_LOAD_OPCODE2(ARM::t2LDRDi8, D, new LoadInfo2(-1, 1, 2))
  // REGISTER_LOAD_OPCODE(t2LDRDi8, t2LDRDi8)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRD_PRE, DPre, new LoadInfo2(-1, 0, 1))
  // REGISTER_LOAD_OPCODE(t2LDRD_PRE, t2LDRD_PRE)
  REGISTER_LOAD_OPCODE2(ARM::t2LDRD_POST, DPost, new LoadInfo2(-1, 0, 1))
  // REGISTER_LOAD_OPCODE(t2LDRD_POST, t2LDRD_POST)
}

void LoadLifter::doD(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(getGlobalContext(), 32);

  c4 = ConstantInt::get(getGlobalContext(), APInt(32, StringRef("4"), 10));

  LoadInfo2* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  Rd = UpdateRd(Rd, Offset, IRB, true);

  unsigned i = 0;
  for (SDNode::use_iterator I = N->use_begin(), E = N->use_end(); I != E; ++I) {
    if (i >= 1 && I->getOpcode() == ISD::CopyToReg) {
      SDNode* succ = *I;

      Value* Rn = ReadReg(Rd, IRB, info->width);
      saveNodeValue(N, Rn);

      visit(succ, IRB);

      if (i < 2)
        Rd = UpdateRd(Rd, getConstant("4"), IRB, true);
      else
        break;
    }
    i++;
  }
}

void LoadLifter::doDPre(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  Value* Rn = NULL;
  uint32_t index;

  LoadInfo2* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  std::string AddrRegName = getReg(N->getOperand(index).getNode());
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  Rd = UpdateRd(Rd, Offset, IRB, true);

  SDNode* node = LookUpSDNode(N, AddrRegName);
  if (node != NULL) {
    saveNodeValue(N, Rd);
    visit(node, IRB);
  }

  node = getFirstOutput(N);
  if (node != NULL) {
    Rn = ReadReg(Rd, IRB, info->width);
    saveNodeValue(N, Rn);
    visit(node, IRB);
    Rd = UpdateRd(Rd, getConstant("4"), IRB, true);
  }

  Rn = ReadReg(Rd, IRB, info->width);
  saveNodeValue(N, Rn);
}

void LoadLifter::doDPost(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  Value* Rn = NULL;
  uint32_t index;

  LoadInfo2* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  std::string AddrRegName = getReg(N->getOperand(index).getNode());
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);
  Value* Rd_init = Rd;

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  // Return the first output Node chained with Noreg
  SDNode* node = getFirstOutput(N);
  if (node != NULL) {
    Rn = ReadReg(Rd, IRB, info->width);
    saveNodeValue(N, Rn);
    visit(node, IRB);
    Rd = UpdateRd(Rd, getConstant("4"), IRB, true);
  }

  // Return the first output Node which is chained with the previous node
  node = getNextOutput(N, node);
  if (node != NULL) {
    Rn = ReadReg(Rd, IRB, info->width);
    saveNodeValue(N, Rn);
    visit(node, IRB);
  }

  node = LookUpSDNode(N, AddrRegName);
  if (node != NULL) {
    Rd = UpdateRd(Rd_init, Offset, IRB, true);
    saveNodeValue(N, Rd);
    visit(node, IRB);
  }
}

void LoadLifter::doPop(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;
  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(getGlobalContext(), 32);

  c4 = ConstantInt::get(getGlobalContext(), APInt(32, StringRef("4"), 10));

  LoadInfo2* info = getInfo(N->getMachineOpcode());

  info->iRn_max = N->getNumOperands() - 1;
  info->iRd = N->getNumOperands() - 1;
  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    Rd = UpdateRd(Rd, c4, IRB, false);

    Value* value = ReadReg(Rd, IRB, info->width);

    Rn = visit(N->getOperand(index).getNode(), IRB);

    Rn = WriteReg(Rn, value, IRB, info->width);
  }

  saveNodeValue(N, Rd);
}

void LoadLifter::doMulti(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;
  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(getGlobalContext(), 32);

  c4 = ConstantInt::get(getGlobalContext(), APInt(32, StringRef("4"), 10));

  LoadInfo2* info = getInfo(N->getMachineOpcode());

  info->iRn_max = N->getNumOperands();
  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    SDNode* pred = N->getOperand(index).getNode();

    Value* Dest = visitRegister(pred->getOperand(1).getNode(), IRB);

    Value* value = ReadReg(Rd, IRB, info->width);

    Rn = WriteReg(value, Dest, IRB, info->width);

    if (info->hasManyUses()) Rd = UpdateRd(Rd, c4, IRB, true);
  }

  saveNodeValue(N, Rd);
}

void LoadLifter::doMultiDB(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;
  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(getGlobalContext(), 32);

  c4 = ConstantInt::get(getGlobalContext(), APInt(32, StringRef("4"), 10));

  LoadInfo2* info = getInfo(N->getMachineOpcode());

  info->iRn_max = N->getNumOperands();
  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  uint32_t j = info->iRn_max - 1 - info->iRn;
  uint32_t inv_rn[j];
  while ((index = info->getNext()) != -1) inv_rn[j--] = index;

  Value* Rn = NULL;
  for (auto i = 0; i < (info->iRn_max - info->iRn); i++) {
    index = inv_rn[i];

    // llvm::errs() << " At index " << i << " = " << index << "\n";

    Rd = UpdateRd(Rd, c4, IRB, false);

    SDNode* pred = N->getOperand(index).getNode();

    Value* Dest = visitRegister(pred->getOperand(1).getNode(), IRB);

    Value* value = ReadReg(Rd, IRB, info->width);

    Rn = WriteReg(value, Dest, IRB, info->width);
  }

  saveNodeValue(N, Rd);
}

void LoadLifter::doPost(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(getGlobalContext(), 32);

  c4 = ConstantInt::get(getGlobalContext(), APInt(32, StringRef("4"), 10));

  LoadInfo2* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  std::string AddrRegName = getReg(N->getOperand(index).getNode());
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);
  Value* Rd_temp = Rd;

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  Value* Rn = ReadReg(Rd, IRB, info->width);

  // Use 1 : Rn
  {
    SDNode* succ = LookUpSDNode(N, AddrRegName);
    if (succ != NULL) {
      Rd = UpdateRd(Rd, Offset, IRB, true);

      saveNodeValue(N, Rd);
      visit(succ, IRB);
      saveNodeValue(N, Rn);
    } else
      saveNodeValue(N, Rd);
  }
}

void LoadLifter::doPre(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(getGlobalContext(), 32);

  c4 = ConstantInt::get(getGlobalContext(), APInt(32, StringRef("4"), 10));

  LoadInfo2* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  std::string AddrRegName = getReg(N->getOperand(index).getNode());
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  Rd = UpdateRd(Rd, Offset, IRB, true);

  Value* Rn = ReadReg(Rd, IRB, info->width);

  // Use 1 : Rn
  {
    SDNode* succ = LookUpSDNode(N, AddrRegName);
    if (succ != NULL) {
      saveNodeValue(N, Rd);
      visit(succ, IRB);
      saveNodeValue(N, Rn);
    } else
      saveNodeValue(N, Rd);
  }
}

void LoadLifter::doSigned(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(getGlobalContext(), 32);

  c4 = ConstantInt::get(getGlobalContext(), APInt(32, StringRef("4"), 10));

  LoadInfo2* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  if (info->shifted) {
    Value* Op = visit(N->getOperand(3).getNode(), IRB);
    Offset = IRB->CreateShl(Offset, Op);
  }

  Rd = UpdateRd(Rd, Offset, IRB, true);

  Value* Rn = ReadReg(Rd, IRB, info->width);

  saveNodeValue(N, Rn);
}

void LoadLifter::doCommon(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;
  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(getGlobalContext(), 32);

  c4 = ConstantInt::get(alm->Mod->getContext(), APInt(32, StringRef("4"), 10));

  LoadInfo2* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  unsigned OpCode = N->getMachineOpcode();
  switch (OpCode) {
    case ARM::tLDRi:
      Offset = IRB->CreateShl(Offset, getConstant("2"));
      break;
    case ARM::tLDRHi:
      Offset = IRB->CreateShl(Offset, getConstant("1"));
      break;
  }

  if (info->shifted) {
    Value* Op = visit(N->getOperand(3).getNode(), IRB);
    Offset = IRB->CreateShl(Offset, Op);
  }

  Rd = UpdateRd(Rd, Offset, IRB, true);

  Value* Rn = NULL;

  Rn = ReadReg(Rd, IRB, info->width);

  saveNodeValue(N, Rn);
}

void LoadLifter::t2LDRD_POSTHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 1, 0);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, true, true, true, layout, true, false, false,
                                NULL, false, true);

  // Retrieve Address
  Value* SavedAddr;
  Value* Addr;
  Value* Res;

  SavedAddr = Addr = visit(info->N->getOperand(layout->Addr).getNode(), IRB);
  Value* Addr_int = Addr;

  std::string AddrRegName =
      getReg(info->N->getOperand(info->Layout->Addr).getNode());

  unsigned i = 0;
  for (SDNode::use_iterator I = info->N->use_begin(), E = info->N->use_end();
       I != E; ++I) {
    if (i++ < info->N->getNumOperands() && I->getOpcode() == ISD::CopyToReg) {
      SDNode* pred = *I;

      // Check if we output the address or the readsVirtualRegister
      std::string DestRegName = getReg(pred);

      if (DestRegName.find(AddrRegName) != std::string::npos) continue;

      Addr = IncPointer(info, IRB, Addr);

      info->Increment = false;

      // Load value
      Res = CreateLoad(info, IRB, Addr);

      alm->VisitMap[info->N] = Res;

      visit(pred, IRB);

      // if (i < info->N->getNumOperands() - 1)
      //   Addr = IncPointer(info, IRB, Addr);
    }
  }

  if (info->OutputDst && info->OutputAddr) {
    std::string AddrRegName =
        getReg(info->N->getOperand(info->Layout->Addr).getNode());

    for (SDNode::use_iterator I = info->N->use_begin(), E = info->N->use_end();
         I != E; ++I) {
      if (I->getOpcode() == ISD::CopyToReg) {
        SDNode* pred = *I;

        // Check if we output the address or the readsVirtualRegister
        std::string DestRegName = getReg(pred);

        if (DestRegName.find(AddrRegName) != std::string::npos) {
          Addr = UpdateAddress(info, IRB);

          alm->VisitMap[info->N] = Addr;

          visit(pred, IRB);
        } else {
          alm->VisitMap[info->N] = Res;
          visit(pred, IRB);
        }
      }
    }
  }
}

void LoadLifter::t2LDRB_POSTHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  Type* Ty = IntegerType::get(getGlobalContext(), 8);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info =
      new LoadInfo(N, false, true, true, layout, true, false, true, Ty);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRHi8Handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  Type* Ty = IntegerType::get(getGlobalContext(), 16);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info =
      new LoadInfo(N, false, false, true, layout, true, true, true, Ty, false);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRD_PREHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 1, 0);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, true, true, true, layout, true, true);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRHsHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1, 3);

  Type* Ty = IntegerType::get(getGlobalContext(), 16);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info =
      new LoadInfo(N, false, false, true, layout, true, true, true, Ty, true);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRBsHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1, 3);

  Type* Ty = IntegerType::get(getGlobalContext(), 8);

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
  LoadInfo* info = new LoadInfo(N, false, false, true, layout, true, true,
                                false, NULL, true);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRH_POSTHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  Type* Ty = IntegerType::get(getGlobalContext(), 16);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info =
      new LoadInfo(N, false, true, true, layout, true, false, true, Ty);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRH_PREHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  Type* Ty = IntegerType::get(getGlobalContext(), 16);

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

  Type* Ty = IntegerType::get(getGlobalContext(), 8);

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

  Type* Ty = IntegerType::get(getGlobalContext(), 16);

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

  Type* Ty = IntegerType::get(getGlobalContext(), 8);

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

  Type* Ty = IntegerType::get(getGlobalContext(), 8);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info =
      new LoadInfo(N, false, false, true, layout, true, true, true, Ty);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDRBi8Handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(-1, -1, 2, 1);

  Type* Ty = IntegerType::get(getGlobalContext(), 8);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info =
      new LoadInfo(N, false, false, true, layout, true, true, true, Ty);

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
  LoadNodeLayout* layout = new LoadNodeLayout(4, max, -1, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, true, true, false, layout, true, false);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDMIAHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(4, max, -1, 1);

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
  LoadInfo* info = new LoadInfo(N, false, false, true, layout, true, true);

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
  LoadNodeLayout* layout = new LoadNodeLayout(max - 1, 4, -1, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, true, true, false, layout, false, true);

  LifteNode(info, IRB);
}

void LoadLifter::t2LDMDBHandler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t max = N->getNumOperands();

  // Dst_start Dst_end Offset Addr
  LoadNodeLayout* layout = new LoadNodeLayout(max - 1, 4, -1, 1);

  // SDNode, MultiDest, OutputAddr, OutputDst, Layout, Increment, Before
  LoadInfo* info = new LoadInfo(N, true, false, false, layout, false, true);

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
    bool db = (layout->Dst_start > layout->Dst_end);

    for (unsigned i = layout->Dst_start;
         db ? i >= layout->Dst_end : i < layout->Dst_end; db ? --i : ++i) {
      // Retrieve destination register
      // Value* Op = visit(info->N->getOperand(i).getNode(), IRB);
      if (info->Before) Addr = IncPointer(info, IRB, Addr);

      SDNode* pred = info->N->getOperand(i).getNode();
      Value* Op = visitRegister(pred->getOperand(1).getNode(), IRB);

      // Load value
      Res = CreateLoad(info, IRB, Addr);

      if (!info->OutputDst) Res = CreateStore(info, IRB, Op, Res);

      // Increment SP
      if (!info->Before) Addr = IncPointer(info, IRB, Addr);
      // Addr_int = Addr;
    }
  } else {
    std::string AddrRegName =
        getReg(info->N->getOperand(info->Layout->Addr).getNode());

    unsigned i = 0;
    for (SDNode::use_iterator I = info->N->use_begin(), E = info->N->use_end();
         I != E; ++I) {
      if (i++ < info->N->getNumOperands() && I->getOpcode() == ISD::CopyToReg) {
        SDNode* pred = *I;

        // Check if we output the address or the readsVirtualRegister
        std::string DestRegName = getReg(pred);

        if (DestRegName.find(AddrRegName) != std::string::npos) continue;

        if (info->Before) Addr = IncPointer(info, IRB, Addr);

        info->Increment = false;

        // Load value
        Res = CreateLoad(info, IRB, Addr);

        alm->VisitMap[info->N] = Res;

        visit(pred, IRB);

        if (!info->Before && i < info->N->getNumOperands() - 1)
          Addr = IncPointer(info, IRB, Addr);
      }
    }
  }

  if (info->Trunc) {
    Res = IRB->CreateTrunc(Res, info->Ty);

    Type* Ty = IntegerType::get(getGlobalContext(), 32);

    Res = IRB->CreateZExt(Res, Ty);
  }

  if (info->OutputDst && !info->OutputAddr) alm->VisitMap[info->N] = Res;
  if (!info->OutputDst && info->OutputAddr) alm->VisitMap[info->N] = Addr;
  if (info->OutputDst && info->OutputAddr) {
    std::string AddrRegName =
        getReg(info->N->getOperand(info->Layout->Addr).getNode());

    for (SDNode::use_iterator I = info->N->use_begin(), E = info->N->use_end();
         I != E; ++I) {
      if (I->getOpcode() == ISD::CopyToReg) {
        SDNode* pred = *I;

        // Check if we output the address or the readsVirtualRegister
        std::string DestRegName = getReg(pred);

        if (DestRegName.find(AddrRegName) != std::string::npos) {
          if (info->Post || (!info->Before && layout->Offset != -1)) {
            Addr = UpdateAddress(info, IRB);
          }

          alm->VisitMap[info->N] = Addr;
          // alm->VisitMap[info->N] = Addr_int;
          visit(pred, IRB);
        } else {
          alm->VisitMap[info->N] = Res;
          visit(pred, IRB);
        }
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
    i = info->Layout->Shift;
    Value* Op = visit(info->N->getOperand(i).getNode(), IRB);

    Offset = IRB->CreateShl(Offset, Op);
  }

  // Add Offset to Address
  Addr = dyn_cast<Instruction>(IRB->CreateAdd(Addr, Offset));
  dyn_cast<Instruction>(Addr)->setDebugLoc(info->N->getDebugLoc());

  return Addr;
}

llvm::Value* LoadLifter::CreateLoad(LoadInfo* info, IRBuilder<>* IRB,
                                    Value* Addr) {
  if (!Addr->getType()->isPointerTy()) {
    Addr = IRB->CreateIntToPtr(Addr, Addr->getType()->getPointerTo());
    (dyn_cast<Instruction>(Addr))->setDebugLoc(info->N->getDebugLoc());
  }

  Value* load = IRB->CreateLoad(Addr);
  dyn_cast<Instruction>(load)->setDebugLoc(info->N->getDebugLoc());

  return load;
}

llvm::Value* LoadLifter::CreateStore(LoadInfo* info, IRBuilder<>* IRB,
                                     Value* Addr, Value* Src) {
  if (!Addr->getType()->isPointerTy()) {
    Addr = IRB->CreateIntToPtr(Addr, Addr->getType()->getPointerTo());
  }

  Instruction* store = IRB->CreateStore(Src, Addr);
  store->setDebugLoc(info->N->getDebugLoc());

  return store;
}

llvm::Value* LoadLifter::IncPointer(LoadInfo* info, IRBuilder<>* IRB,
                                    Value* Addr) {
  ConstantInt* const_4 =
      ConstantInt::get(getGlobalContext(), APInt(32, StringRef("4"), 10));

  if (info->Increment)
    Addr = dyn_cast<Instruction>(IRB->CreateAdd(Addr, const_4));
  else
    Addr = dyn_cast<Instruction>(IRB->CreateSub(Addr, const_4));

  dyn_cast<Instruction>(Addr)->setDebugLoc(info->N->getDebugLoc());

  return Addr;
}
