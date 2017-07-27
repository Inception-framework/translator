#include "Target/ARM/ARMLifter.h"
#include "Target/ARM/ARMLifterManager.h"

#define DEBUG_TYPE "ARMLifter.cpp"

using namespace llvm;

Value* ARMLifter::Bool2Int(Value* v, IRBuilder<>* IRB) {
  auto& C = alm->Mod->getContext();

  auto bool_ty = llvm::Type::getInt1Ty(C);
  auto int32_ty = llvm::Type::getInt32Ty(C);

  if (v->getType() != bool_ty) {
    v = IRB->CreateTrunc(v, bool_ty);
  }
  v = IRB->CreateZExt(v, int32_ty);

  return v;
}

Value* ARMLifter::Reg(StringRef name) {
  Value* Reg = alm->Mod->getGlobalVariable(name);

  if (Reg == NULL) {
    Type* Ty = IntegerType::get(alm->Mod->getContext(), 32);

    Constant* Initializer = Constant::getNullValue(Ty);

    Reg = new GlobalVariable(*alm->Mod,  // Module
                             Ty,         // Type
                             false,      // isConstant
                             GlobalValue::CommonLinkage, Initializer, name);
  }

  return Reg;
}

Value* ARMLifter::getConstant(StringRef value) {
  ConstantInt* constante =
      ConstantInt::get(alm->Mod->getContext(), APInt(32, value, 10));

  return constante;
}

Value* ARMLifter::ReadAddress(Value* Rd, Type* Ty, IRBuilder<>* IRB) {
  Type* Ty_word = IntegerType::get(alm->Mod->getContext(), 16);

  if (!Rd->getType()->isPointerTy()) {
    Rd = IRB->CreateIntToPtr(Rd, Rd->getType()->getPointerTo());
  }

  if (Ty != NULL && Ty != Ty_word) {
    Rd = IRB->CreateTrunc(Rd, Ty);
    Rd = IRB->CreateZExt(Rd, Ty_word);
  }

  Rd = IRB->CreateLoad(Rd);

  return Rd;
}

Value* ARMLifter::ReadReg(Value* Rn, IRBuilder<>* IRB) {
  Instruction* store = IRB->CreateLoad(Rn);

  return store;
}

Value* ARMLifter::WriteReg(Value* Rn, Value* Rd, Type* Ty, IRBuilder<>* IRB) {
  Type* Ty_word = IntegerType::get(alm->Mod->getContext(), 16);

  if (!Rd->getType()->isPointerTy()) {
    Rd = IRB->CreateIntToPtr(Rd, Rd->getType()->getPointerTo());
  }

  if (Ty != NULL && Ty != Ty_word) {
    Rn = IRB->CreateTrunc(Rn, Ty);

    Rn = IRB->CreateZExt(Rn, Ty_word);
  }

  Instruction* store = IRB->CreateStore(Rn, Rd);

  return store;
}

Value* ARMLifter::UpdateRd(Value* Rn, Value* Offset, IRBuilder<>* IRB,
                           bool Increment) {
  uint32_t i;

  // Add Offset to Address
  if (Increment)
    Rn = IRB->CreateAdd(Rn, Offset);
  else
    Rn = IRB->CreateSub(Rn, Offset);

  return Rn;
}

Value* ARMLifter::saveNodeValue(SDNode* N, Value* Rn) { alm->VisitMap[N] = Rn; }

Value* ARMLifter::visit(const SDNode* N, IRBuilder<>* IRB) {
  if (alm->VisitMap.find(N) != alm->VisitMap.end()) {
    return alm->VisitMap[N];
  }

  IRB->SetCurrentDebugLocation(N->getDebugLoc());

  // DEBUG(Infos << "Visiting Node: ");
  // DEBUG(N->print(Infos));
  // DEBUG(Infos << "\n");
  // DEBUG(Infos << format("%1" PRIx64,
  // Dec->getDisassembler()->getDebugOffset(N->getDebugLoc())) << "\n");

  switch (N->getOpcode()) {
    default: {
      errs() << "OpCode: " << N->getOpcode() << "\n";
      N->dump();
      llvm_unreachable(
          "IREmitter::visit - Every visit should be implemented...");
      return NULL;
    }
    case ISD::EntryToken:
      return NULL;
    // case ISD::HANDLENODE:         EndHandleDAG = true;
    case ISD::UNDEF:
      return NULL;
    case ISD::CopyFromReg:
      return visitCopyFromReg(N, IRB);
    case ISD::CopyToReg:
      return visitCopyToReg(N, IRB);
    case ISD::Constant:
      return visitConstant(N);
  }
  return NULL;
}

std::string ARMLifter::getReg(const SDNode* N) {
  const RegisterSDNode* R =
      dyn_cast<RegisterSDNode>(N->getOperand(1).getNode());
  if (R == NULL) {
    errs() << "visitRegister with no register!?\n";
    return NULL;
  }

  std::string RegName;
  raw_string_ostream RP(RegName);

  RP << PrintReg(R->getReg(), alm->RegisterInfo);

  RegName = RP.str().substr(1, RP.str().size());

  return RegName;
}

Value* ARMLifter::visitRegister(const SDNode* N, IRBuilder<>* IRB) {
  const RegisterSDNode* R = dyn_cast<RegisterSDNode>(N);
  if (R == NULL) {
    errs() << "visitRegister with no register!?\n";
    return NULL;
  }

  Value* Reg = alm->RegMap[R->getReg()];
  if (Reg == NULL) {
    // Regname is %regname when printed this way.
    std::string RegName;
    raw_string_ostream RP(RegName);

    RP << PrintReg(R->getReg(), alm->RegisterInfo);

    RegName = RP.str().substr(1, RP.str().size());

    if (RegName.find("noreg") != std::string::npos) return NULL;

    Type* Ty = R->getValueType(0).getTypeForEVT(alm->Mod->getContext());

    Reg = alm->Mod->getGlobalVariable(RegName);
    if (Reg == NULL) {
      Constant* Initializer = Constant::getNullValue(Ty);

      GlobalVariable* gvar_ptr =
          new GlobalVariable(*alm->Mod,  // Module
                             Ty,         // Type
                             false,      // isConstant
                             GlobalValue::CommonLinkage, Initializer, RegName);

      if (RegName.find("SP") != std::string::npos) {
        // Initializer = ConstantInt::get(
        // Dec->getModule()->getContext(), APInt(32,
        // StringRef("268435456"), 10));
        //
        // gvar_ptr->setInitializer(Initializer);

        // XXX: SP cannot be initialized with other than 0
        // XXX: Therefore, We introduce a store before the first
        // instruction
        Function* main_fct = alm->Mod->getFunction("main");
        if (!main_fct)
          llvm::errs() << "Unable to find main function needed to init SP !";

        ConstantInt* const_int32 = ConstantInt::get(
            alm->Mod->getContext(), APInt(32, StringRef("536875008"), 10));

        Instruction* inst = main_fct->getEntryBlock().begin();

        IRBuilder<>* builder = new IRBuilder<>(inst);
        builder->CreateStore(const_int32, gvar_ptr);
      }

      gvar_ptr->setAlignment(4);
      Reg = gvar_ptr;
    }
    alm->RegMap[R->getReg()] = Reg;
  }
  return Reg;
}

Value* ARMLifter::visitCopyFromReg(const SDNode* N, IRBuilder<>* IRB) {
  // Operand 0 - Chain node (ignored)
  // Operand 1 - RegisterSDNode, a machine register. We create an alloca,
  // which
  //             is typically removed in a mem2reg pass

  // Skip if the register is never used. This happens for %noreg
  // registers.
  if (!N->hasAnyUseOfValue(0)) {
    return NULL;
  }

  Value* RegVal = visitRegister(N->getOperand(1).getNode(), IRB);
  if (RegVal == NULL) {
    errs() << "visitCopyFromReg: Invalid Register!\n";
    return NULL;
  }

  Instruction* Res = IRB->CreateLoad(RegVal);
  alm->VisitMap[N] = Res;
  Res->setDebugLoc(N->getDebugLoc());
  return Res;
}

Value* ARMLifter::visitCopyToReg(const SDNode* N, IRBuilder<>* IRB) {
  // Operand 0 - Chain node (ignored)
  // Operand 1 - Register Destination
  // Operand 2 - Source
  Value* RegVal = visitRegister(N->getOperand(1).getNode(), IRB);
  Value* V = visit(N->getOperand(2).getNode(), IRB);

  if (V == NULL || RegVal == NULL) {
    errs() << "Null values on CopyToReg, skipping!\n";
    return NULL;
  }

  if (!RegVal->getType()->isPointerTy()) {
    RegVal =
        IRB->CreateIntToPtr(RegVal, RegVal->getType()->getPointerTo());
  }

  Instruction* Res = IRB->CreateStore(V, RegVal);
  alm->VisitMap[N] = Res;
  Res->setDebugLoc(N->getDebugLoc());
  return Res;
}

Value* ARMLifter::visitConstant(const SDNode* N) {
  if (const ConstantSDNode* CSDN = dyn_cast<ConstantSDNode>(N)) {
    Value* Res = Constant::getIntegerValue(
        N->getValueType(0).getTypeForEVT(alm->Mod->getContext()),
        CSDN->getAPIntValue());
    alm->VisitMap[N] = Res;
    return Res;
  } else {
    llvm::errs() << "Could not convert ISD::Constant to integer!\n";
  }
  return NULL;
}
