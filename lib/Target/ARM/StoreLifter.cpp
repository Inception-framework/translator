#include "Target/ARM/StoreLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void StoreLifter::registerLifter() {
#define REGISTER_STORE_OPCODE(opcode, handler, composition)               \
  alm->registerLifter(this, std::string("StoreLifter"), (unsigned)opcode, \
                      (LifterHandler)&StoreLifter::do##handler);          \
  info.insert(std::pair<unsigned, StoreInfo*>((unsigned)opcode, composition));

  REGISTER_STORE_OPCODE(ARM::tPUSH, Push, new StoreInfo(3, -1, 0))
  REGISTER_STORE_OPCODE(ARM::t2STMIA_UPD, Multi, new StoreInfo(4, 1, 0))
  REGISTER_STORE_OPCODE(ARM::t2STMIA, Multi, new StoreInfo(4, 1, 0))

  REGISTER_STORE_OPCODE(ARM::t2STMDB_UPD, MultiDB, new StoreInfo(4, 1, 0))
  REGISTER_STORE_OPCODE(ARM::t2STMDB, MultiDB, new StoreInfo(4, 1, 0))

  REGISTER_STORE_OPCODE(ARM::tSTRi, Common, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::tSTRspi, Common, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::tSTRr, Common, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::t2STRi8, Common, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::t2STRi12, Common, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::t2STR_PRE, Pre, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::t2STR_POST, Post, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::t2STRs, Signed, new StoreInfo(1, 2, 3))

  REGISTER_STORE_OPCODE(ARM::tSTRBi, Common, new StoreInfo(1, 2, 3, 8))
  REGISTER_STORE_OPCODE(ARM::tSTRBr, Common, new StoreInfo(1, 2, 3, 8))
  REGISTER_STORE_OPCODE(ARM::t2STRBi8, Common, new StoreInfo(1, 2, 3, 8))
  REGISTER_STORE_OPCODE(ARM::t2STRBi12, Common, new StoreInfo(1, 2, 3, 8))
  REGISTER_STORE_OPCODE(ARM::t2STRB_PRE, Pre, new StoreInfo(1, 2, 3, 8))
  REGISTER_STORE_OPCODE(ARM::t2STRB_POST, Post, new StoreInfo(1, 2, 3, 8))
  REGISTER_STORE_OPCODE(ARM::t2STRBs, Signed, new StoreInfo(1, 2, 3, 8))

  REGISTER_STORE_OPCODE(ARM::tSTRHi, Common, new StoreInfo(1, 2, 3, 16))
  REGISTER_STORE_OPCODE(ARM::tSTRHr, Common, new StoreInfo(1, 2, 3, 16))
  REGISTER_STORE_OPCODE(ARM::t2STRHi12, Common, new StoreInfo(1, 2, 3, 16))
  REGISTER_STORE_OPCODE(ARM::t2STRHi8, Common, new StoreInfo(1, 2, 3, 16))
  REGISTER_STORE_OPCODE(ARM::t2STRH_PRE, Pre, new StoreInfo(1, 2, 3, 16))
  REGISTER_STORE_OPCODE(ARM::t2STRH_POST, Post, new StoreInfo(1, 2, 3, 16))
  REGISTER_STORE_OPCODE(ARM::t2STRHs, Signed, new StoreInfo(1, 2, 3, 16))

  REGISTER_STORE_OPCODE(ARM::t2STRDi8, D, new StoreInfo(1, 3, 4, 32, 3))
  REGISTER_STORE_OPCODE(ARM::t2STRD_PRE, Pre, new StoreInfo(0, 2, 3, 32, 2))
  REGISTER_STORE_OPCODE(ARM::t2STRD_POST, Post, new StoreInfo(0, 2, 3, 32, 2))
}

void StoreLifter::doMultiDB(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  StoreInfo* info = getInfo(N->getMachineOpcode());

  info->iRn_max = N->getNumOperands();
  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);
  Value* Rd_init = Rd;

  // Value* Rn = NULL;
  // while ((index = info->getNext()) != -1) {
  //
  //   Rd = UpdateRd(Rd, getConstant("4"), IRB, false);
  //
  //   Rn = visit(N->getOperand(index).getNode(), IRB);
  //
  //   Rn = WriteReg(Rn, Rd, IRB, info->width);
  // }

  uint32_t j = info->iRn_max - info->iRn - 1;
  uint32_t inv_rn[j];
  while ((index = info->getNext()) != -1) inv_rn[j--] = index;

  Value* Rn = NULL;
  for (auto i = 0; i < (info->iRn_max - info->iRn); i++) {
    index = inv_rn[i];

    llvm::errs() << " At index " << i << " = " << index << "\n";

    Rd = UpdateRd(Rd, getConstant("4"), IRB, false);

    Rn = visit(N->getOperand(index).getNode(), IRB);

    Rn = WriteReg(Rn, Rd, IRB, info->width);
  }

  if (N->getMachineOpcode() == ARM::t2STMDB_UPD)
    saveNodeValue(N, Rd);
  else
    saveNodeValue(N, Rn);
}

void StoreLifter::doD(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  StoreInfo* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  Rd = UpdateRd(Rd, Offset, IRB, true);

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    Rn = visit(N->getOperand(index).getNode(), IRB);

    Rn = WriteReg(Rn, Rd, IRB, info->width);

    if (info->hasManyUses()) Rd = UpdateRd(Rd, getConstant("4"), IRB, true);
  }

  saveNodeValue(N, Rn);
}

void StoreLifter::doPush(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;
  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->getContextRef(), 32);

  c4 = ConstantInt::get(alm->getContextRef(), APInt(32, StringRef("4"), 10));

  StoreInfo* info = getInfo(N->getMachineOpcode());

  info->iRn_max = N->getNumOperands() - 1;
  info->iRd = N->getNumOperands() - 1;
  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    Rd = UpdateRd(Rd, c4, IRB, true);

    Rn = visit(N->getOperand(index).getNode(), IRB);

    Rn = WriteReg(Rn, Rd, IRB, info->width);
  }

  saveNodeValue(N, Rd);
}

void StoreLifter::doMulti(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;
  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->getContextRef(), 32);

  c4 = ConstantInt::get(alm->getContextRef(), APInt(32, StringRef("4"), 10));

  StoreInfo* info = getInfo(N->getMachineOpcode());

  info->iRn_max = N->getNumOperands();
  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    Rn = visit(N->getOperand(index).getNode(), IRB);

    Rn = WriteReg(Rn, Rd, IRB, info->width);

    if (info->hasManyUses()) Rd = UpdateRd(Rd, c4, IRB, true);
  }

  saveNodeValue(N, Rd);
}

void StoreLifter::doPost(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->getContextRef(), 32);

  c4 = ConstantInt::get(alm->getContextRef(), APInt(32, StringRef("4"), 10));

  StoreInfo* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);
  Value* Rd_temp = Rd;

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    Rn = visit(N->getOperand(index).getNode(), IRB);

    Rn = WriteReg(Rn, Rd_temp, IRB, info->width);

    if (info->hasManyUses()) Rd_temp = UpdateRd(Rd_temp, c4, IRB, false);
  }

  Rd = UpdateRd(Rd, Offset, IRB, true);

  saveNodeValue(N, Rd);
}

void StoreLifter::doPre(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->getContextRef(), 32);

  c4 = ConstantInt::get(alm->getContextRef(), APInt(32, StringRef("4"), 10));

  StoreInfo* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  Rd = UpdateRd(Rd, Offset, IRB, true);
  Value* Rd_temp = Rd;

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    Rn = visit(N->getOperand(index).getNode(), IRB);

    Rn = WriteReg(Rn, Rd_temp, IRB, info->width);

    if (info->hasManyUses()) Rd_temp = UpdateRd(Rd_temp, c4, IRB, true);
  }

  saveNodeValue(N, Rd);
}

void StoreLifter::doSigned(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->getContextRef(), 32);

  c4 = ConstantInt::get(alm->getContextRef(), APInt(32, StringRef("4"), 10));

  StoreInfo* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  Value* Op = visit(N->getOperand(4).getNode(), IRB);
  Offset = IRB->CreateShl(Offset, Op);

  Rd = UpdateRd(Rd, Offset, IRB, true);
  Value* Rd_temp = Rd;

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    Rn = visit(N->getOperand(index).getNode(), IRB);

    Rn = WriteReg(Rn, Rd_temp, IRB, info->width);

    if (info->hasManyUses()) Rd_temp = UpdateRd(Rd_temp, c4, IRB, true);
  }

  saveNodeValue(N, Rd);
}

void StoreLifter::doCommon(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;
  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->getContextRef(), 32);

  c4 = ConstantInt::get(alm->getContextRef(), APInt(32, StringRef("4"), 10));

  StoreInfo* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  unsigned opcode = N->getMachineOpcode();
  switch (opcode) {
    case ARM::tSTRi:
      Offset = IRB->CreateMul(Offset, getConstant("4"));
  }

  Rd = UpdateRd(Rd, Offset, IRB, true);

  Value* Rn = NULL;
  Rn = visit(N->getOperand(info->iRn).getNode(), IRB);

  Rn = WriteReg(Rn, Rd, IRB, info->width);

  saveNodeValue(N, Rn);
}
