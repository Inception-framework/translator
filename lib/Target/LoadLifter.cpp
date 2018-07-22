
/*
    This file is part of Inception translator.

    Inception translator is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Inception translator.  If not, see <https://www.gnu.org/licenses/>.

    Copyright (c) 2017 Maxim Integrated, Inc.
    Author: Nassim Corteggiani <nassim.corteggiani@maximintegrated.com>

    Copyright (c) 2017 EURECOM, Inc.
    Author: Giovanni Camurati <giovanni.camurati@eurecom.fr>
*/

#include "Target/ARM/LoadLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void LoadLifter::registerLifter() {
#define REGISTER_LOAD_OPCODE(opcode, handler, composition)               \
  alm->registerLifter(this, std::string("LoadLifter"), (unsigned)opcode, \
                      (LifterHandler)&LoadLifter::do##handler);          \
  info.insert(std::pair<unsigned, LoadInfo*>((unsigned)opcode, composition));

  REGISTER_LOAD_OPCODE(ARM::tPOP, Pop, new LoadInfo(3, -1, 0))

  REGISTER_LOAD_OPCODE(ARM::t2LDMIA_UPD, Multi, new LoadInfo(4, 1, 0))
  REGISTER_LOAD_OPCODE(ARM::tLDMIA_UPD, Multi, new LoadInfo(4, 1, 0))
  REGISTER_LOAD_OPCODE(ARM::tLDMIA, Multi, new LoadInfo(4, 1, 0))
  REGISTER_LOAD_OPCODE(ARM::t2LDMIA, Multi, new LoadInfo(4, 1, 0))

  REGISTER_LOAD_OPCODE(ARM::t2LDMDB_UPD, MultiDB, new LoadInfo(4, 1, -1, 32, 0))

  REGISTER_LOAD_OPCODE(ARM::t2LDMDB, MultiDB, new LoadInfo(4, 1, -1, 32, 0))
  // t2LDREXD	= 2391,

  REGISTER_LOAD_OPCODE(ARM::tLDRi, Common, new LoadInfo(-1, 1, 2))
  REGISTER_LOAD_OPCODE(ARM::tLDRspi, Common, new LoadInfo(-1, 1, 2))
  REGISTER_LOAD_OPCODE(ARM::tLDRpci, PC, new LoadInfo(-1, -1, 1))
  REGISTER_LOAD_OPCODE(ARM::t2LDRpci, PC, new LoadInfo(-1, -1, 1))
  REGISTER_LOAD_OPCODE(ARM::tLDRr, Common, new LoadInfo(-1, 1, 2))
  REGISTER_LOAD_OPCODE(ARM::t2LDRi8, Common, new LoadInfo(-1, 1, 2))
  REGISTER_LOAD_OPCODE(ARM::t2LDRi12, Common, new LoadInfo(-1, 1, 2))
  REGISTER_LOAD_OPCODE(ARM::t2LDRs, Signed, new LoadInfo(1, 1, 2, true))
  REGISTER_LOAD_OPCODE(ARM::t2LDR_PRE, Pre, new LoadInfo(-1, 1, 2))
  REGISTER_LOAD_OPCODE(ARM::t2LDR_POST, Post, new LoadInfo(-1, 1, 2))
  REGISTER_LOAD_OPCODE(ARM::t2LDREX, Common, new LoadInfo(-1, 1, 2))

  REGISTER_LOAD_OPCODE(ARM::tLDRBi, Common, new LoadInfo(-1, 1, 2, 8))
  REGISTER_LOAD_OPCODE(ARM::tLDRBr, Common, new LoadInfo(-1, 1, 2, 8))
  REGISTER_LOAD_OPCODE(ARM::t2LDRBi8, Common, new LoadInfo(-1, 1, 2, 8))
  REGISTER_LOAD_OPCODE(ARM::t2LDRBi12, Common, new LoadInfo(-1, 1, 2, 8))
  REGISTER_LOAD_OPCODE(ARM::t2LDRSBi12, Common, new LoadInfo(-1, 1, 2, 8))
  REGISTER_LOAD_OPCODE(ARM::t2LDRB_PRE, Pre, new LoadInfo(-1, 1, 2, 8))
  REGISTER_LOAD_OPCODE(ARM::t2LDRB_POST, Post, new LoadInfo(-1, 1, 2, 8))
  REGISTER_LOAD_OPCODE(ARM::t2LDRBs, Common, new LoadInfo(-1, 1, 2, 8, true))
  REGISTER_LOAD_OPCODE(ARM::t2LDREXB, Common, new LoadInfo(-1, 1, 2, 8))

  REGISTER_LOAD_OPCODE(ARM::tLDRHi, Common, new LoadInfo(-1, 1, 2, 16))
  REGISTER_LOAD_OPCODE(ARM::tLDRHr, Common, new LoadInfo(-1, 1, 2, 16))
  REGISTER_LOAD_OPCODE(ARM::tLDRSH, Common, new LoadInfo(-1, 1, 2, 16))
  REGISTER_LOAD_OPCODE(ARM::t2LDRSHi12, Common, new LoadInfo(-1, 1, 2, 16))
  REGISTER_LOAD_OPCODE(ARM::t2LDRHi12, Common, new LoadInfo(-1, 1, 2, 16))
  REGISTER_LOAD_OPCODE(ARM::t2LDRHi8, Common, new LoadInfo(-1, 1, 2, 16))
  REGISTER_LOAD_OPCODE(ARM::t2LDRH_PRE, Pre, new LoadInfo(-1, 1, 2, 16))
  REGISTER_LOAD_OPCODE(ARM::t2LDRH_POST, Post, new LoadInfo(-1, 1, 2, 16))
  REGISTER_LOAD_OPCODE(ARM::t2LDRHs, Common, new LoadInfo(-1, 1, 2, 16, true))
  REGISTER_LOAD_OPCODE(ARM::t2LDREXH, Common, new LoadInfo(-1, 1, 2, 16))

  REGISTER_LOAD_OPCODE(ARM::t2LDRDi8, D, new LoadInfo(-1, 1, 2))
  REGISTER_LOAD_OPCODE(ARM::t2LDRD_PRE, DPre, new LoadInfo(-1, 0, 1))
  REGISTER_LOAD_OPCODE(ARM::t2LDRD_POST, DPost, new LoadInfo(-1, 0, 1))
}

void LoadLifter::doPC(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  int32_t index;

  LoadInfo* info = getInfo(N->getMachineOpcode());

  const Disassembler* Dis = alm->Dec->getDisassembler();

  uint32_t debugLoc = Dis->getDebugOffset(N->getDebugLoc());
  debugLoc += 4;

  index = info->iOffset;
  SDNode* Node = N->getOperand(index).getNode();
  ConstantSDNode* OffsetNode = dyn_cast<ConstantSDNode>(Node);
  if (OffsetNode == NULL) {
    inception_error("[LoadLifter] DoPC Handler expected ConstantSDNode ...");
    return;
  }

  int64_t offset = OffsetNode->getZExtValue();

  FractureMemoryObject* fmo = Dis->getCurSectionMemory();

  if (!fmo->isValidAddress(debugLoc + offset)) {
    inception_error(
        "[LoadLifter] DoPC encountered a memory adress out of "
        "current section ...");
    return;
  }

  uint8_t byte;
  uint32_t value = 0;
  uint64_t address = (debugLoc + offset);
  address &= ~(1 << 1);
  for (int i = 0; i < 4; i++) {
    if (fmo->readByte(&byte, address + i) == -1) {
      inception_error(
          "[LoadLifter] DoPC encountered a memory adress out of "
          "current section ...");
      return;
    } else {
      value |= byte << (i * 8);
    }
  }

  Value* Rd = ConstantInt::get(IContext::getContextRef(), APInt(32, value));

  saveNodeValue(N, Rd);
}

void LoadLifter::doD(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  int32_t index;

  LoadInfo* info = getInfo(N->getMachineOpcode());

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
  int32_t index;

  LoadInfo* info = getInfo(N->getMachineOpcode());

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
  int32_t index;

  LoadInfo* info = getInfo(N->getMachineOpcode());

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
  int32_t index;
  ConstantInt* c4;

  c4 = ConstantInt::get(IContext::getContextRef(),
                        APInt(32, StringRef("4"), 10));

  LoadInfo* info = getInfo(N->getMachineOpcode());

  info->iRn_max = N->getNumOperands() - 1;
  info->iRd = N->getNumOperands() - 1;
  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    Value* value = ReadReg(Rd, IRB, info->width);

    Rn = visitRegister(N->getOperand(index).getNode()->getOperand(1).getNode(),
                       IRB);

    Rn = WriteReg(value, Rn, IRB, info->width);

    Rd = UpdateRd(Rd, c4, IRB, true);
  }

  saveNodeValue(N, Rd);
}

void LoadLifter::doMulti(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  int32_t index;
  ConstantInt* c4;

  c4 = ConstantInt::get(IContext::getContextRef(),
                        APInt(32, StringRef("4"), 10));

  LoadInfo* info = getInfo(N->getMachineOpcode());

  bool writeback = true;

  info->iRn_max = N->getNumOperands();
  index = info->iRd;
  SDNode* Rn = N->getOperand(index).getNode();
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);
  Value* Rd_init = visit(N->getOperand(index)->getOperand(1).getNode(), IRB);

  while ((index = info->getNext()) != -1) {
    SDNode* pred = N->getOperand(index).getNode();

    if (getReg(Rn).compare(getReg(pred)) == 0) writeback = false;

    Value* Dest = visitRegister(pred->getOperand(1).getNode(), IRB);

    Value* value = ReadReg(Rd, IRB, info->width);

    WriteReg(value, Dest, IRB, info->width);

    if (info->hasManyUses()) Rd = UpdateRd(Rd, c4, IRB, true);
  }

  if (N->getMachineOpcode() == ARM::tLDMIA) {
    // We need to check if tLDMIA is in the registers list
    // if it is, writeback is enabled
    if (writeback) Rd = WriteReg(Rd, Rd_init, IRB, info->width, false);
  }

  saveNodeValue(N, Rd);
}

void LoadLifter::doMultiDB(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  int32_t index;
  ConstantInt* c4;

  c4 = ConstantInt::get(IContext::getContextRef(),
                        APInt(32, StringRef("4"), 10));

  LoadInfo* info = getInfo(N->getMachineOpcode());

  info->iRn_max = N->getNumOperands();
  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  uint32_t j = info->iRn_max - info->iRn;
  uint32_t* inv_rn = new uint32_t[j];
  while ((index = info->getNext()) != -1) {
    inv_rn[--j] = index;
  }

  for (auto i = 0; i < (info->iRn_max - info->iRn); i++) {
    index = inv_rn[i];

    Rd = UpdateRd(Rd, c4, IRB, false);

    SDNode* pred = N->getOperand(index).getNode();

    Value* Dest = visitRegister(pred->getOperand(1).getNode(), IRB);

    Value* value = ReadReg(Rd, IRB, info->width);

    WriteReg(value, Dest, IRB, info->width);
  }

  saveNodeValue(N, Rd);

  delete[] inv_rn;
}

void LoadLifter::doPost(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  int32_t index;

  LoadInfo* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  std::string AddrRegName = getReg(N->getOperand(index).getNode());
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

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
  int32_t index;

  LoadInfo* info = getInfo(N->getMachineOpcode());

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
  int32_t index;

  LoadInfo* info = getInfo(N->getMachineOpcode());

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
  int32_t index;

  LoadInfo* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  unsigned OpCode = N->getMachineOpcode();
  switch (OpCode) {
    case ARM::tLDRi:
    case ARM::tLDRspi:
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

  switch (OpCode) {
    case ARM::tLDRSH:
    case ARM::t2LDRSBi12:
    case ARM::t2LDRSHi12:
      // some instructions need sign extension
      Rn = ReadReg(Rd, IRB, info->width, true);
      break;
    default:
      Rn = ReadReg(Rd, IRB, info->width);
      break;
  }

  saveNodeValue(N, Rn);
}
