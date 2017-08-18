#include "Target/ARM/ARMLifter.h"
#include "Target/ARM/ARMLifterManager.h"

#include "ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#define DEBUG_TYPE "ARMLifter.cpp"

using namespace llvm;

SDNode* ARMLifter::LookUpSDNode(SDNode* N, std::string name) {
  unsigned i = 0;
  for (SDNode::use_iterator I = N->use_begin(), E = N->use_end(); I != E; ++I) {
    if (i++ < N->getNumOperands() && I->getOpcode() == ISD::CopyToReg) {
      SDNode* succ = *I;

      std::string DestRegName = getReg(succ);

      if (DestRegName.find(name) == std::string::npos)
        continue;
      else
        return succ;
    }
  }

  return NULL;
}

bool ARMLifter::IsSigned(SDNode* N) {
  llvm::errs() << "\n\n\nLooking for signed node \n";

  for (SDNode::use_iterator U = N->use_begin(), EU = N->use_end(); U != EU; ++U)
    for (SDNode::op_iterator O = U->op_begin(), EO = U->op_end(); O != EO; ++O)
      if (IsCPSR(O->getNode())) {
        outs() << "Found CPSR in output\n";
        return true;
      }

  for (SDNode::op_iterator O = N->op_begin(), EO = N->op_end(); O != EO; ++O)
    for (SDNode::use_iterator U = O->getNode()->use_begin(),
                              EU = O->getNode()->use_end();
         U != EU; ++U) {
      SDNode* node = dyn_cast<SDNode>(*U);
      if (IsCPSR(node)) {
        outs() << "Found CPSR in input\n";
        return true;
      }
    }

  return false;
}

// TODO: This code is likely to contain some bugs
// TODO: In particular: behavior inside an IT block and Add rd,rn
bool ARMLifter::IsSetFlags(SDNode* N) {
  if (N->isMachineOpcode()) {
    switch (N->getMachineOpcode()) {
      //
      // check the S bit
      //
      case ARM::tADDspi:
      case ARM::tADDframe:
      case ARM::tADDspr:
      case ARM::t2ADDSri:
      case ARM::t2ADDSrr:
      case ARM::t2ADDSrs:
      case ARM::t2ADDri:
      case ARM::t2ADDrr:
      case ARM::t2ADDrs:
      case ARM::t2ADCri:
      case ARM::t2ADCrr:
      case ARM::tADC:

      case ARM::t2LSLri:
      case ARM::t2LSLrr:
      case ARM::t2LSRri:
      case ARM::t2LSRrr:
      case ARM::t2ASRri:
      case ARM::t2ASRrr:
      case ARM::t2RORri:
      case ARM::t2RORrr:
        if (IsCPSR(N->getOperand(4).getNode()->getOperand(1).getNode())) {
          outs() << "IsSetFlags = true\n";
          return true;
        } else {
          outs() << "IsSetFlags = false\n";
          return false;
        }
      case ARM::t2RRX:
      case ARM::t2MOVr:  // TODO other mov, this is the one necessary for lsl 0
      case ARM::t2MOVi:
      case ARM::t2MVNr:
      case ARM::t2MVNi:
        if (IsCPSR(N->getOperand(3).getNode()->getOperand(1).getNode())) {
          outs() << "IsSetFlags = true\n";
          return true;
        } else {
          outs() << "IsSetFlags = false\n";
          return false;
        }
      //
      // always true outside block
      //
      case ARM::tADDrr:
      case ARM::tADDi8:
      case ARM::tADDi3:
      case ARM::tLSLri:
      case ARM::tLSLrr:
      case ARM::tLSRri:
      case ARM::tLSRrr:
      case ARM::tASRri:
      case ARM::tASRrr:
      case ARM::tROR:
      case ARM::tMOVSr:
      case ARM::tMOVi8:
      case ARM::tMVN:
        // TODO more cases
        // TODO should we also check if outside IT block and not AL condition
        // for some of them?
        outs() << "IsSetFlags = true\n";
        return true;
      //
      // alsways false
      //
      case ARM::t2ADDri12:
      case ARM::tADDhirr:
      case ARM::tADDrSP:
      case ARM::tADDrSPi:
      default:
        outs() << "IsSetFlags = false\n";
        return false;
    }
    }
    outs() << "IsSetFlags = false\n";
    return false;
}

bool ARMLifter::IsCPSR(SDNode* N) {
  N->dump();

  if (N->getOpcode() == ISD::Register) {
    const RegisterSDNode* R = dyn_cast<RegisterSDNode>(N);
    if (R == NULL) {
      return false;
    }

    std::string RegName;
    raw_string_ostream RP(RegName);

    RP << PrintReg(R->getReg(), alm->RegisterInfo);

    RegName = RP.str().substr(1, RP.str().size());

    llvm::errs() << " Found -" << RegName << "- \n";

    if (RegName.compare("CPSR") == 0) {
      llvm::errs() << " Match \n";
      return true;
    }
    return false;
  }
  return false;
}

bool ARMLifter::IsPC(SDNode* N) {
  N->dump();

  if (N->getOpcode() == ISD::Register) {
    const RegisterSDNode* R = dyn_cast<RegisterSDNode>(N);
    if (R == NULL) {
      return false;
    }

    std::string RegName;
    raw_string_ostream RP(RegName);

    RP << PrintReg(R->getReg(), alm->RegisterInfo);

    RegName = RP.str().substr(1, RP.str().size());

    llvm::errs() << " Found -" << RegName << "- \n";

    if (RegName.compare("PC") == 0) {
      llvm::errs() << " Match \n";
      return true;
    }
    return false;
  }
  return false;
}

Value* ARMLifter::Bool2Int(Value* v, IRBuilder<>* IRB) {
  auto bool_ty = llvm::Type::getInt1Ty(alm->getContextRef());
  auto int32_ty = llvm::Type::getInt32Ty(alm->getContextRef());

  if (v->getType() != bool_ty) {
    v = IRB->CreateTrunc(v, bool_ty);
  }
  v = IRB->CreateZExt(v, int32_ty);

  return v;
}

Value* ARMLifter::Reg(StringRef name) {
  Value* Reg = alm->Mod->getGlobalVariable(name);

  if (Reg == NULL) {
    Type* Ty = IntegerType::get(alm->getContextRef(), 32);

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
      ConstantInt::get(alm->getContextRef(), APInt(32, value, 10));

  return constante;
}

Value* ARMLifter::ReadAddress(Value* Rd, Type* Ty, IRBuilder<>* IRB) {
  Type* Ty_word = IntegerType::get(alm->getContextRef(), 32);

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

Value* ARMLifter::ReadReg(Value* Rn, IRBuilder<>* IRB, int Width) {
  Type* Ty = NULL;

  switch (Width) {
    case 8:
      Ty = IntegerType::get(alm->getContextRef(), 8);
      break;
    case 16:
      Ty = IntegerType::get(alm->getContextRef(), 16);
      break;
    default:
      Ty = Ty = IntegerType::get(alm->getContextRef(), 32);
      break;
  }

  if (!Rn->getType()->isPointerTy()) {
    Rn = IRB->CreateIntToPtr(Rn, Rn->getType()->getPointerTo());
  }

  Value* load = IRB->CreateLoad(Rn);

  if (Width != 32) {
    load = IRB->CreateTrunc(load, Ty);

    load = IRB->CreateZExt(load, IntegerType::get(alm->getContextRef(), 32));
  }

  return load;
}

Value* ARMLifter::WriteReg(Value* Rn, Value* Rd, IRBuilder<>* IRB, int Width,
                           bool extend) {
  Type* Ty = NULL;

  switch (Width) {
    case 8:
      Ty = IntegerType::get(alm->getContextRef(), 8);
      break;
    case 16:
      Ty = IntegerType::get(alm->getContextRef(), 16);
      break;
    default:
      Ty = Ty = IntegerType::get(alm->getContextRef(), 32);
      break;
  }

  if (!Rd->getType()->isPointerTy()) {
    if (Width != 32 && !extend) {
      Rd = IRB->CreateIntToPtr(Rd, Ty->getPointerTo());
    } else {
      Rd = IRB->CreateIntToPtr(Rd, Rd->getType()->getPointerTo());
    }
  }

  if (Width != 32) {
    Rn = IRB->CreateTrunc(Rn, Ty);

    if (extend) {
      Rn = IRB->CreateZExt(Rn, IntegerType::get(alm->getContextRef(), 32));
    }
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
    // errs() << "visitRegister with no register!?\n";
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
    // errs() << "visitRegister with no register!?\n";
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

    Type* Ty = R->getValueType(0).getTypeForEVT(alm->getContextRef());

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
            alm->getContextRef(), APInt(32, StringRef("536875008"), 10));

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
    // errs() << "visitCopyFromReg: Invalid Register!\n";
    // N->dump();
    return NULL;
  }

  Value* Res = NULL;
  if (IsPC(N->getOperand(1).getNode())) {
    uint64_t address = 0;
    address = alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
    address += 4;
    Value* PCval = ConstantInt::get(
        llvm::Type::getInt32Ty(alm->getContextRef()), address, false);
    Res = PCval;
  } else {
    Res = IRB->CreateLoad(RegVal);
  }
  alm->VisitMap[N] = Res;
  // Res->setDebugLoc(N->getDebugLoc());
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
    RegVal = IRB->CreateIntToPtr(RegVal, RegVal->getType()->getPointerTo());
  }

  Instruction* Res = IRB->CreateStore(V, RegVal);
  alm->VisitMap[N] = Res;
  Res->setDebugLoc(N->getDebugLoc());
  return Res;
}

Value* ARMLifter::visitConstant(const SDNode* N) {
  if (const ConstantSDNode* CSDN = dyn_cast<ConstantSDNode>(N)) {
    Value* Res = Constant::getIntegerValue(
        N->getValueType(0).getTypeForEVT(alm->getContextRef()),
        CSDN->getAPIntValue());
    alm->VisitMap[N] = Res;
    return Res;
  } else {
    llvm::errs() << "Could not convert ISD::Constant to integer!\n";
  }
  return NULL;
}
