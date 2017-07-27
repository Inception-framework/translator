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

  //
  // REGISTER_LOAD_OPCODE(tPUSH, tPUSH)
  //
  // REGISTER_LOAD_OPCODE(t2LDMIA_UPD, t2LDMIA_UPD)
  // REGISTER_LOAD_OPCODE(t2LDMIA, t2LDMIA)
  //
  // REGISTER_LOAD_OPCODE(tSTRr, tSTRr)
  // REGISTER_LOAD_OPCODE(t2LDMDB_UPD, t2LDMDB_UPD)
  // REGISTER_LOAD_OPCODE(t2LDMDB, t2LDMDB)
  //
  // REGISTER_LOAD_OPCODE(t2STRDi8, t2STRDi8)
  REGISTER_STORE_OPCODE(ARM::t2STRD_PRE, Pre, new StoreInfo(0, 2, 3, NULL, 2))
  REGISTER_STORE_OPCODE(ARM::t2STRD_POST, Post, new StoreInfo(0, 2, 3, NULL, 2))

  // REGISTER_LOAD_OPCODE(tSTRi, tSTRi)
  // REGISTER_LOAD_OPCODE(t2STRi8, t2STRi8)
  // REGISTER_LOAD_OPCODE(t2STRi12, t2STRi12)
  // REGISTER_LOAD_OPCODE(t2STRs, t2STRs)
  REGISTER_STORE_OPCODE(ARM::t2STR_PRE, Pre, new StoreInfo(1, 2, 3))
  REGISTER_STORE_OPCODE(ARM::t2STR_POST, Post, new StoreInfo(1, 2, 3))

  // REGISTER_LOAD_OPCODE(tSTRBi, tSTRBi)
  // REGISTER_LOAD_OPCODE(t2STRBi8, t2STRBi8)
  // REGISTER_LOAD_OPCODE(t2STRBi12, t2STRBi12)
  REGISTER_STORE_OPCODE(ARM::t2STRB_PRE, Pre, new StoreInfo(1, 2, 3, Ty_byte))
  REGISTER_STORE_OPCODE(ARM::t2STRB_POST, Post, new StoreInfo(1, 2, 3, Ty_byte))
  // REGISTER_LOAD_OPCODE(t2STRBs, t2STRBs)

  // REGISTER_LOAD_OPCODE(tSTRHi, tSTRHi)
  // REGISTER_LOAD_OPCODE(t2STRHi12, t2STRHi12)
  // REGISTER_LOAD_OPCODE(t2STRHi8, t2STRHi8)
  REGISTER_STORE_OPCODE(ARM::t2STRH_PRE, Pre, new StoreInfo(1, 2, 3, Ty_hword))
  REGISTER_STORE_OPCODE(ARM::t2STRH_POST, Post, new StoreInfo(1, 2, 3, Ty_hword))
  // REGISTER_LOAD_OPCODE(t2STRHs, t2STRHs)
}

void StoreLifter::doPost(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

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
    // Rn = ReadAddress(Rn, info->Ty, IRB);

    Rn = WriteReg(Rn, Rd_temp, info->Ty, IRB);

    if(info->hasManyUses())
      Rd_temp = UpdateRd(Rd_temp, c4, IRB, false);
  }

  Rd = UpdateRd(Rd, Offset, IRB, true);

  saveNodeValue(N, Rd);
}

void StoreLifter::doPre(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
  uint32_t index;

  ConstantInt* c4;

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

    Rn = WriteReg(Rn, Rd_temp, info->Ty, IRB);

    if(info->hasManyUses())
      Rd_temp = UpdateRd(Rd_temp, c4, IRB, true);
  }

  saveNodeValue(N, Rd);
}

// void StoreLifter::t2STRi12Handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {
//
//   // Lift Operands
//   Value* Addr = visit(N->getOperand(2).getNode(), IRB);
//   Value* Offset = visit(N->getOperand(3).getNode(), IRB);
//
//   // Compute Register Value
//   StringRef BaseName = getBaseValueName(Addr->getName());
//   StringRef Name = getIndexedValueName(BaseName);
//
//   // Add Offset to Address
//   Addr = dyn_cast<Instruction>(IRB->CreateSub(Addr, Offset, Name));
//   dyn_cast<Instruction>(Addr)->setDebugLoc(N->getDebugLoc());
//
//   BaseName = getBaseValueName(Addr->getName());
//
//   if (!Addr->getType()->isPointerTy()) {
//     Name = getIndexedValueName(BaseName);
//     Addr = IRB->CreateIntToPtr(Addr, Addr->getType()->getPointerTo(), Name);
//   }
//
//   Value* Op = visit(N->getOperand(1).getNode(), IRB);
//
//   Instruction* store = IRB->CreateStore(Op, Addr);
//   store->setDebugLoc(N->getDebugLoc());
// }
