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

  Type* Ty_byte = IntegerType::get(alm->Mod->getContext(), 8);
  Type* Ty_hword = IntegerType::get(alm->Mod->getContext(), 16);
  Type* Ty_word = IntegerType::get(alm->Mod->getContext(), 32);

  REGISTER_STORE_OPCODE(ARM::tPUSH, Push, new StoreInfo(3, -1, 0))
  // REGISTER_LOAD_OPCODE(, t2LDMIA_UPD)
  REGISTER_STORE_OPCODE(ARM::t2STMIA_UPD, Multi, new StoreInfo(4, 1, 0))
  REGISTER_STORE_OPCODE(ARM::t2STMIA, Multi, new StoreInfo(4, 1, 0))

  REGISTER_STORE_OPCODE(ARM::t2LDMDB_UPD, Multi,
                        new StoreInfo(0, 2, 3, NULL, 2))
  REGISTER_STORE_OPCODE(ARM::t2LDMDB, Multi, new StoreInfo(0, 2, 3, NULL, 2))

  REGISTER_STORE_OPCODE(ARM::tSTRr, Common, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::tSTRi, Common, new StoreInfo(1, 2, 3))

  // REGISTER_LOAD_OPCODE(t2STRDi8, t2STRDi8)
  REGISTER_STORE_OPCODE(ARM::t2STRD_PRE, Pre, new StoreInfo(0, 2, 3, NULL, 2))
  REGISTER_STORE_OPCODE(ARM::t2STRD_POST, Post, new StoreInfo(0, 2, 3, NULL, 2))

  REGISTER_STORE_OPCODE(ARM::t2STRi8, Common, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::t2STRi12, Common, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::t2STR_PRE, Pre, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::t2STR_POST, Post, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::t2STRs, Signed, new StoreInfo(1, 2, 3))

  // REGISTER_LOAD_OPCODE(tSTRBi, tSTRBi)
  // REGISTER_LOAD_OPCODE(t2STRBi8, t2STRBi8)
  // REGISTER_LOAD_OPCODE(t2STRBi12, t2STRBi12)
  REGISTER_STORE_OPCODE(ARM::t2STRB_PRE, Pre, new StoreInfo(1, 2, 3, Ty_byte))
  REGISTER_STORE_OPCODE(ARM::t2STRB_POST, Post, new StoreInfo(1, 2, 3, Ty_byte))
  REGISTER_STORE_OPCODE(ARM::t2STRBs, Signed, new StoreInfo(1, 2, 3, Ty_byte))

  // REGISTER_LOAD_OPCODE(tSTRHi, tSTRHi)
  // REGISTER_LOAD_OPCODE(t2STRHi12, t2STRHi12)
  // REGISTER_LOAD_OPCODE(t2STRHi8, t2STRHi8)
  REGISTER_STORE_OPCODE(ARM::t2STRH_PRE, Pre, new StoreInfo(1, 2, 3, Ty_hword))
  REGISTER_STORE_OPCODE(ARM::t2STRH_POST, Post,
                        new StoreInfo(1, 2, 3, Ty_hword))
  REGISTER_STORE_OPCODE(ARM::t2STRHs, Signed, new StoreInfo(1, 2, 3, Ty_hword))
}

void StoreLifter::doPush(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;
  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->Mod->getContext(), 32);

  c4 = ConstantInt::get(alm->Mod->getContext(), APInt(32, StringRef("4"), 10));

  StoreInfo* info = getInfo(N->getMachineOpcode());

  info->iRn_max = N->getNumOperands() - 1;
  info->iRd = N->getNumOperands() - 1;
  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    Rd = UpdateRd(Rd, c4, IRB, true);

    Rn = visit(N->getOperand(index).getNode(), IRB);

    Rn = WriteReg(Rn, Rd, info->Ty, IRB);
  }

  saveNodeValue(N, Rd);
}

void StoreLifter::doMulti(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;
  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->Mod->getContext(), 32);

  c4 = ConstantInt::get(alm->Mod->getContext(), APInt(32, StringRef("4"), 10));

  StoreInfo* info = getInfo(N->getMachineOpcode());

  info->iRn_max = N->getNumOperands();
  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    Rn = visit(N->getOperand(index).getNode(), IRB);

    Rn = WriteReg(Rn, Rd, info->Ty, IRB);

    if (info->hasManyUses()) Rd = UpdateRd(Rd, c4, IRB, true);
  }

  saveNodeValue(N, Rd);
}

void StoreLifter::doPost(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->Mod->getContext(), 32);

  c4 = ConstantInt::get(alm->Mod->getContext(), APInt(32, StringRef("4"), 10));

  StoreInfo* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);
  Value* Rd_temp = Rd;

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  Value* Rn = NULL;
  while ((index = info->getNext()) != -1) {
    Rn = visit(N->getOperand(index).getNode(), IRB);

    if (info->Ty != NULL && info->Ty != Ty_word) {
      Rn = IRB->CreateTrunc(Rn, info->Ty);
      Rn = IRB->CreateZExt(Rn, Ty_word);
    }
    Rn = WriteReg(Rn, Rd_temp, info->Ty, IRB);

    if (info->hasManyUses()) Rd_temp = UpdateRd(Rd_temp, c4, IRB, false);
  }

  Rd = UpdateRd(Rd, Offset, IRB, true);

  saveNodeValue(N, Rd);
}

void StoreLifter::doPre(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->Mod->getContext(), 32);

  c4 = ConstantInt::get(alm->Mod->getContext(), APInt(32, StringRef("4"), 10));

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

    if (info->Ty != NULL && info->Ty != Ty_word) {
      Rn = IRB->CreateTrunc(Rn, info->Ty);
      Rn = IRB->CreateZExt(Rn, Ty_word);
    }

    Rn = WriteReg(Rn, Rd_temp, info->Ty, IRB);

    if (info->hasManyUses()) Rd_temp = UpdateRd(Rd_temp, c4, IRB, true);
  }

  saveNodeValue(N, Rd);
}

void StoreLifter::doSigned(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->Mod->getContext(), 32);

  c4 = ConstantInt::get(alm->Mod->getContext(), APInt(32, StringRef("4"), 10));

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

    if (info->Ty != NULL && info->Ty != Ty_word) {
      Rn = IRB->CreateTrunc(Rn, info->Ty);
      Rn = IRB->CreateSExt(Rn, Ty_word);
    }

    Rn = WriteReg(Rn, Rd_temp, info->Ty, IRB);

    if (info->hasManyUses()) Rd_temp = UpdateRd(Rd_temp, c4, IRB, true);
  }

  saveNodeValue(N, Rd);
}

void StoreLifter::doCommon(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;
  ConstantInt* c4;

  Type* Ty_word = IntegerType::get(alm->Mod->getContext(), 32);

  c4 = ConstantInt::get(alm->Mod->getContext(), APInt(32, StringRef("4"), 10));

  StoreInfo* info = getInfo(N->getMachineOpcode());

  index = info->iRd;
  Value* Rd = visit(N->getOperand(index).getNode(), IRB);

  index = info->iOffset;
  Value* Offset = visit(N->getOperand(index).getNode(), IRB);

  Rd = UpdateRd(Rd, Offset, IRB, true);

  Value* Rn = NULL;
  Rn = visit(N->getOperand(info->getNext()).getNode(), IRB);

  Rn = WriteReg(Rn, Rd, info->Ty, IRB);

  saveNodeValue(N, Rd);
}
