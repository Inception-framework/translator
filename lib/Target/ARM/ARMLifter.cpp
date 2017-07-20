#include "Target/ARM/ARMLifter.h"
#include "Target/ARM/ARMLifterManager.h"

#define DEBUG_TYPE "ARMLifter.cpp"

using namespace llvm;

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

StringRef ARMLifter::getIndexedValueName(StringRef BaseName) {
  const ValueSymbolTable& ST = alm->Mod->getValueSymbolTable();

  // In the common case, the name is not already in the symbol table.
  Value* V = ST.lookup(BaseName);
  if (V == NULL) {
    return BaseName;
  }

  // Otherwise, there is a naming conflict.  Rename this value.
  // FIXME: AFAIK this is never deallocated (memory leak). It should be free'd
  // after it gets added to the symbol table (which appears to do a copy as
  // indicated by the original code that stack allocated this variable).
  SmallString<256>* UniqueName =
      new SmallString<256>(BaseName.begin(), BaseName.end());
  unsigned Size = BaseName.size();

  // Add '_' as the last character when BaseName ends in a number
  if (BaseName[Size - 1] <= '9' && BaseName[Size - 1] >= '0') {
    UniqueName->resize(Size + 1);
    (*UniqueName)[Size] = '_';
    Size++;
  }

  unsigned LastUnique = 0;
  while (1) {
    // Trim any suffix off and append the next number.
    UniqueName->resize(Size);
    raw_svector_ostream(*UniqueName) << ++LastUnique;

    // Try insert the vmap entry with this suffix.
    V = ST.lookup(*UniqueName);
    // FIXME: ^^ this lookup does not appear to be working on non-globals...
    // Temporary Fix: check if it has a alm->BaseNames entry
    if (V == NULL && alm->BaseNames[*UniqueName].empty()) {
      alm->BaseNames[*UniqueName] = BaseName;
      return *UniqueName;
    }
  }
}

StringRef ARMLifter::getBaseValueName(StringRef BaseName) {
  // Note: An alternate approach would be to pull the Symbol table and
  // do a string search, but this is much easier to implement.
  StringRef Res = alm->BaseNames.lookup(BaseName);
  if (Res.empty()) {
    return BaseName;
  }
  return Res;
}

StringRef ARMLifter::getInstructionName(const SDNode* N, IRBuilder<>* IRB) {
  // Look for register name in CopyToReg user
  for (SDNode::use_iterator I = N->use_begin(), E = N->use_end(); I != E; ++I) {
    if (I->getOpcode() == ISD::CopyToReg) {
      return getIndexedValueName(
          visitRegister(I->getOperand(1).getNode(), IRB)->getName());
      // FIXME - Favor the first result number.  (EFLAGS vs ESI x86)
    }
  }
  return StringRef();
}

std::string ARMLifter::getReg(const SDNode* N)  {

  const RegisterSDNode* R = dyn_cast<RegisterSDNode>(N->getOperand(1).getNode());
  if (R == NULL) {
    errs() << "visitRegister with no register!?\n";
    return NULL;
  }

  std::string RegName;
  raw_string_ostream RP(RegName);

  RP << PrintReg(R->getReg(),
                 alm->RegisterInfo);

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

    if(RegName.find("noreg")!=std::string::npos)
      return NULL;

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
        // Dec->getModule()->getContext(), APInt(32, StringRef("268435456"),
        // 10));
        //
        // gvar_ptr->setInitializer(Initializer);

        // XXX: SP cannot be initialized with other than 0
        // XXX: Therefore, We introduce a store before the first instruction
        Function* main_fct = alm->Mod->getFunction("main");
        if (!main_fct)
          llvm::errs() << "Unable to find main function needed to init SP !";

        ConstantInt* const_int32 = ConstantInt::get(
            alm->Mod->getContext(), APInt(32, StringRef("268435456"), 10));

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
  // Operand 1 - RegisterSDNode, a machine register. We create an alloca, which
  //             is typically removed in a mem2reg pass

  // Skip if the register is never used. This happens for %noreg registers.
  if (!N->hasAnyUseOfValue(0)) {
    return NULL;
  }

  Value* RegVal = visitRegister(N->getOperand(1).getNode(), IRB);
  if (RegVal == NULL) {
    errs() << "visitCopyFromReg: Invalid Register!\n";
    return NULL;
  }

  StringRef Name = getIndexedValueName(RegVal->getName());
  Instruction* Res = IRB->CreateLoad(RegVal, Name);
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

  // errs() << "V:\t"
  //        << "Output Type: " << V->getType()->getTypeID() << "\n";
  // V->dump();
  // errs() << "RegVal:\t"
  //        << "Output Type: " << RegVal->getType()->getTypeID() << "\n";
  // RegVal->dump();
  // errs() << "\n\n";

  StringRef BaseName = getBaseValueName(RegVal->getName());
  StringRef Name = getIndexedValueName(BaseName);

  if (!RegVal->getType()->isPointerTy()) {
    RegVal =
        IRB->CreateIntToPtr(RegVal, RegVal->getType()->getPointerTo(), Name);
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
