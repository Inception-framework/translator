#include "Target/ARM/StoreLifter.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

void StoreLifter::registerLifter() {
  #define REGISTER_STORE_OPCODE(opcode, handler)                                             \
    alm->registerLifter(this, std::string("StoreLifter"), (unsigned)ARM::opcode, \
                        (LifterHandler)&StoreLifter::handler##Handler);

    REGISTER_STORE_OPCODE(t2STRi12, t2STRi12)
}

void StoreLifter::t2STRi12Handler(llvm::SDNode* N, llvm::IRBuilder<>* IRB) {

  // Lift Operands
  Value* Addr = visit(N->getOperand(2).getNode(), IRB);
  Value* Offset = visit(N->getOperand(3).getNode(), IRB);

  // Compute Register Value
  StringRef BaseName = getBaseValueName(Addr->getName());
  StringRef Name = getIndexedValueName(BaseName);

  // Add Offset to Address
  Addr = dyn_cast<Instruction>(IRB->CreateSub(Addr, Offset, Name));
  dyn_cast<Instruction>(Addr)->setDebugLoc(N->getDebugLoc());

  BaseName = getBaseValueName(Addr->getName());

  if (!Addr->getType()->isPointerTy()) {
    Name = getIndexedValueName(BaseName);
    Addr = IRB->CreateIntToPtr(Addr, Addr->getType()->getPointerTo(), Name);
  }

  Value* Op = visit(N->getOperand(1).getNode(), IRB);

  Instruction* store = IRB->CreateStore(Op, Addr);
  store->setDebugLoc(N->getDebugLoc());
}
