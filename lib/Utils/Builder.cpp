
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

#include "Utils/Builder.h"
#include "Utils/ABIAdapter.h"

#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace inception;
using namespace fracture;

void initAPI(llvm::Module* _Mod, Decompiler* DEC) {
  IContext::Mod = _Mod;
  IContext::alm->Dec = DEC;
}

std::string getReg(const SDNode* N) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  const RegisterSDNode* R =
      dyn_cast<RegisterSDNode>(N->getOperand(1).getNode());
  if (R == NULL) {
    // errs() << "visitRegister with no register!?\n";
    return NULL;
  }

  std::string RegName;
  raw_string_ostream RP(RegName);

  RP << PrintReg(R->getReg(), IContext::RegisterInfo);

  RegName = RP.str().substr(1, RP.str().size());

  return RegName;
}

SDNode* LookUpSDNode(SDNode* N, std::string name) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

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

bool IsCPSR(SDNode* N) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  if (N->getOpcode() == ISD::Register) {
    const RegisterSDNode* R = dyn_cast<RegisterSDNode>(N);
    if (R == NULL) {
      return false;
    }

    std::string RegName;
    raw_string_ostream RP(RegName);

    RP << PrintReg(R->getReg(), IContext::RegisterInfo);

    RegName = RP.str().substr(1, RP.str().size());

    if (RegName.compare("CPSR") == 0) {
      return true;
    }
    return false;
  }
  return false;
}

bool IsPC(SDNode* N) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  if (N->getOpcode() == ISD::Register) {
    const RegisterSDNode* R = dyn_cast<RegisterSDNode>(N);
    if (R == NULL) {
      return false;
    }

    std::string RegName;
    raw_string_ostream RP(RegName);

    RP << PrintReg(R->getReg(), IContext::RegisterInfo);

    RegName = RP.str().substr(1, RP.str().size());

    if (RegName.compare("PC") == 0) {
      return true;
    }
    return false;
  }
  return false;
}

bool IsITSTATE(SDNode* N) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  if (N->getOpcode() == ISD::Register) {
    const RegisterSDNode* R = dyn_cast<RegisterSDNode>(N);
    if (R == NULL) {
      return false;
    }

    std::string RegName;
    raw_string_ostream RP(RegName);

    RP << PrintReg(R->getReg(), IContext::RegisterInfo);

    RegName = RP.str().substr(1, RP.str().size());

    if (RegName.compare("ITSTATE") == 0) {
      return true;
    }
    return false;
  }
  return false;
}

bool IsSigned(SDNode* N) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  for (SDNode::use_iterator U = N->use_begin(), EU = N->use_end(); U != EU; ++U)
    for (SDNode::op_iterator O = U->op_begin(), EO = U->op_end(); O != EO; ++O)
      if (IsCPSR(O->getNode())) {
        return true;
      }

  for (SDNode::op_iterator O = N->op_begin(), EO = N->op_end(); O != EO; ++O)
    for (SDNode::use_iterator U = O->getNode()->use_begin(),
                              EU = O->getNode()->use_end();
         U != EU; ++U) {
      SDNode* node = dyn_cast<SDNode>(*U);
      if (IsCPSR(node)) {
        return true;
      }
    }

  return false;
}

// TODO: This code is likely to contain some bugs
// TODO: In particular: behavior inside an IT block and Add rd,rn
bool IsSetFlags(SDNode* N) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  if (N->isMachineOpcode()) {
    switch (N->getMachineOpcode()) {
      //
      // check the S bit
      //
      case ARM::t2ADDSrs:
      case ARM::t2ADDrs:
      case ARM::t2ADCrs:

      case ARM::t2SUBSrs:
      case ARM::t2SUBrs:
      case ARM::t2SBCrs:

      case ARM::t2ANDrs:
      case ARM::t2EORrs:
      case ARM::t2ORRrs:
      case ARM::t2ORNrs:
      case ARM::t2BICrs:
        if (IsCPSR(N->getOperand(5).getNode()->getOperand(1).getNode())) {
          return true;
        } else {
          return false;
        }

      case ARM::tADDframe:
      case ARM::t2ADDSri:
      case ARM::t2ADDSrr:
      case ARM::t2ADDri:
      case ARM::t2ADDrr:
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

      case ARM::t2SUBrr:
      case ARM::t2SUBri:
      case ARM::t2SBCrr:
      case ARM::tSBC:
      case ARM::t2SBCri:
      case ARM::t2SUBSri:
      case ARM::t2SUBSrr:

      case ARM::t2RSBri:

      case ARM::t2MVNs:

      case ARM::t2ANDri:
      case ARM::t2ANDrr:

      case ARM::t2EORri:
      case ARM::t2EORrr:

      case ARM::t2ORRri:
      case ARM::t2ORRrr:

      case ARM::t2ORNri:
      case ARM::t2ORNrr:

      case ARM::t2BICri:
      case ARM::t2BICrr:
        if (IsCPSR(N->getOperand(4).getNode()->getOperand(1).getNode())) {
          return true;
        } else {
          return false;
        }
      case ARM::t2RRX:
      case ARM::t2MOVr:  // TODO other mov, this is the one necessary for lsl 0
      case ARM::t2MOVi:
      case ARM::t2MVNr:
      case ARM::t2MVNi:
        if (IsCPSR(N->getOperand(3).getNode()->getOperand(1).getNode())) {
          return true;
        } else {
          return false;
        }
      //
      // always true outside block, always false inside
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

      case ARM::tSUBrr:
      case ARM::tSUBi8:
      case ARM::tSUBi3:

      case ARM::tAND:
      case ARM::tEOR:
      case ARM::tORR:
      case ARM::tBIC:

      case ARM::tRSB:
        // TODO more cases
        // TODO should we also check if outside IT block and not AL condition
        // for some of them?
        if ((IContext::alm->Dec->it_state & 0b1111) == 0) {
          return true;
        } else {
          return false;
        }
      //
      // alsways false
      //
      case ARM::tADDspi:
      case ARM::tADDspr:
      case ARM::t2ADDri12:
      case ARM::tADDhirr:
      case ARM::tADDrSP:
      case ARM::tADDrSPi:

      case ARM::t2MOVi16:
      case ARM::tMOVr:

      case ARM::t2SUBri12:
      case ARM::tSUBspi:
      default:
        return false;
    }
  }
  return false;
}

Value* Bool2Int(Value* v, IRBuilder<>* IRB) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  auto bool_ty = llvm::Type::getInt1Ty(IContext::getContextRef());
  auto int32_ty = llvm::Type::getInt32Ty(IContext::getContextRef());

  if (v->getType() != bool_ty) {
    v = IRB->CreateTrunc(v, bool_ty);
  }
  v = IRB->CreateZExt(v, int32_ty);

  return v;
}

Value* Reg(StringRef name) {
  Type* Ty_word = IntegerType::get(IContext::getContextRef(), 32);

  return Reg(name, Ty_word);
}

Value* Reg(StringRef name, Type* Ty) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  Value* Reg = IContext::Mod->getGlobalVariable(name);

  if (Reg == NULL) {
    Constant* Initializer = Constant::getNullValue(Ty);

    Reg = new GlobalVariable(*IContext::Mod,  // Module
                             Ty,              // Type
                             false,           // isConstant
                             GlobalValue::CommonLinkage, Initializer, name);
  }

  return Reg;
}

Value* getConstant(StringRef value, int Width) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  ConstantInt* constante =
      ConstantInt::get(IContext::getContextRef(), APInt(Width, value, 10));

  return constante;
}

Value* getConstant(StringRef value) { return getConstant(value, 32); }

Value* getConstant(uint32_t value) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  ConstantInt* constante =
      ConstantInt::get(IContext::getContextRef(), APInt(32, value));

  return constante;
}

Value* ReadReg(Value* Rn, IRBuilder<>* IRB, int Width, bool Sign) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");
  Type* Ty = NULL;

  switch (Width) {
    case 8:
      Ty = IntegerType::get(IContext::getContextRef(), 8);
      break;
    case 16:
      Ty = IntegerType::get(IContext::getContextRef(), 16);
      break;
    default:
      Ty = IntegerType::get(IContext::getContextRef(), 32);
      break;
  }

  if (!Rn->getType()->isPointerTy()) {
    Rn = IRB->CreateIntToPtr(Rn, Rn->getType()->getPointerTo());
  }

  Rn = IRB->CreateBitCast(Rn, Ty->getPointerTo());

  Value* load = IRB->CreateLoad(Rn);

  if (Width != 32) {
    load = IRB->CreateTrunc(load, Ty);

    if (Sign) {
      load = IRB->CreateSExt(load,
                             IntegerType::get(IContext::getContextRef(), 32));
    } else {
      load = IRB->CreateZExt(load,
                             IntegerType::get(IContext::getContextRef(), 32));
    }
  }

  return load;
}

Value* ReadReg(Value* Rn, IRBuilder<>* IRB, int Width) {
  return ReadReg(Rn, IRB, Width, false);
}
Value* ReadReg(Value* Rn, IRBuilder<>* IRB) {
  return ReadReg(Rn, IRB, 32, false);
}

Value* WriteReg(Value* Rn, Value* Rd, IRBuilder<>* IRB, int Width,
                bool extend) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");
  Type* Ty = NULL;

  switch (Width) {
    case 8:
      Ty = IntegerType::get(IContext::getContextRef(), 8);
      break;
    case 16:
      Ty = IntegerType::get(IContext::getContextRef(), 16);
      break;
    default:
      Ty = IntegerType::get(IContext::getContextRef(), 32);
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
      Rn = IRB->CreateZExt(Rn, IntegerType::get(IContext::getContextRef(), 32));
    }
  }

  Instruction* store = IRB->CreateStore(Rn, Rd);

  return store;
}

Value* WriteReg(Value* Rn, Value* Rd, IRBuilder<>* IRB) {
  return WriteReg(Rn, Rd, IRB, 32, false);
}

Value* WriteReg(Value* Rn, Value* Rd, IRBuilder<>* IRB, int Width) {
  return WriteReg(Rn, Rd, IRB, Width, false);
}

Value* UpdateRd(Value* Rn, Value* Offset, IRBuilder<>* IRB, bool Increment) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  // Add Offset to Address
  if (Increment)
    Rn = IRB->CreateAdd(Rn, Offset);
  else
    Rn = IRB->CreateSub(Rn, Offset);

  return Rn;
}

void saveNodeValue(const SDNode* N, Value* Rn) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  IContext::VisitMap[N] = Rn;
}

Value* getSavedValue(const SDNode* N) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  return IContext::VisitMap[N];
}

llvm::Module* getModule() {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  return IContext::Mod;
}

Value* getInteger(int value) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  return ConstantInt::get(llvm::Type::getInt32Ty(IContext::getContextRef()),
                          value, false);
}

std::string getRegisterSDNodeName(const RegisterSDNode* R) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  // Regname is %regname when printed this way.
  std::string RegName;
  raw_string_ostream RP(RegName);

  RP << PrintReg(R->getReg(), IContext::RegisterInfo);

  // We remove the % char
  RegName = RP.str().substr(1, RP.str().size());

  return RegName;
}

Value* createCondition(int cond, IRBuilder<>* IRB) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  ARMCC::CondCodes ARMcc = ARMCC::CondCodes(cond);
  if (ARMcc == ARMCC::AL) {
    // do nothing if unconditional
    return NULL;
  }

  // create condition and branch
  Value* Cmp = NULL;
  Value* Cmp1 = NULL;
  Value* Cmp2 = NULL;
  switch (ARMcc) {
    default:
      errs() << "Unknown condition code\n";
      return NULL;
    case ARMCC::EQ:
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("1"));
      break;
    case ARMCC::NE:
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("0"));
      break;
    case ARMCC::HS:
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("CF"), IRB), getConstant("1"));
      break;
    case ARMCC::LO:
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("CF"), IRB), getConstant("0"));
      break;
    case ARMCC::MI:
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("NF"), IRB), getConstant("1"));
      break;
    case ARMCC::PL:
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("NF"), IRB), getConstant("0"));
      break;
    case ARMCC::VS:
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("VF"), IRB), getConstant("1"));
      break;
    case ARMCC::VC:
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("VF"), IRB), getConstant("0"));
      break;
    case ARMCC::HI:
      Cmp1 = IRB->CreateICmpEQ(ReadReg(Reg("CF"), IRB), getConstant("1"));
      Cmp2 = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("0"));
      Cmp = IRB->CreateAnd(Cmp1, Cmp2);
      break;
    case ARMCC::LS:
      Cmp1 = IRB->CreateICmpEQ(ReadReg(Reg("CF"), IRB), getConstant("0"));
      Cmp2 = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("1"));
      Cmp = IRB->CreateOr(Cmp1, Cmp2);
      break;
    case ARMCC::GE:
      Cmp = IRB->CreateICmpEQ(ReadReg(Reg("NF"), IRB), ReadReg(Reg("VF"), IRB));
      break;
    case ARMCC::LT:
      Cmp = IRB->CreateICmpNE(ReadReg(Reg("NF"), IRB), ReadReg(Reg("VF"), IRB));
      break;
    case ARMCC::GT:
      Cmp1 = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("0"));
      Cmp2 =
          IRB->CreateICmpEQ(ReadReg(Reg("NF"), IRB), ReadReg(Reg("VF"), IRB));
      Cmp = IRB->CreateAnd(Cmp1, Cmp2);
      break;
    case ARMCC::LE:
      Cmp1 = IRB->CreateICmpEQ(ReadReg(Reg("ZF"), IRB), getConstant("1"));
      Cmp2 =
          IRB->CreateICmpNE(ReadReg(Reg("NF"), IRB), ReadReg(Reg("VF"), IRB));
      Cmp = IRB->CreateOr(Cmp1, Cmp2);
      break;
  }
  return Cmp;
}

Constant* GetVoidFunctionPointer(StringRef function_name) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  std::vector<Type*> FuncTy_args;
  FunctionType* FuncTy = FunctionType::get(
      /*Result=*/Type::getVoidTy(IContext::getContextRef()),
      /*Params=*/FuncTy_args,
      /*isVarArg=*/true);

  PointerType* PointerTy = PointerType::get(FuncTy, 0);

  // Function Declarations
  Function* function = IContext::Mod->getFunction(function_name);
  if (!function) {
    function = Function::Create(
        /*Type=*/FuncTy,
        /*Linkage=*/GlobalValue::ExternalLinkage,
        /*Name=*/function_name, IContext::Mod);
    function->setCallingConv(CallingConv::C);
  }

  // Constant Definitions
  Constant* fct_ptr =
      ConstantExpr::getCast(Instruction::BitCast, function, PointerTy);

  return fct_ptr;
}

Constant* GetIntIntFunctionPointer(StringRef function_name) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  // Type Definitions
  std::vector<Type*> FuncTy_args;
  FuncTy_args.push_back(IntegerType::get(IContext::getContextRef(), 32));
  FuncTy_args.push_back(IntegerType::get(IContext::getContextRef(), 32));
  FunctionType* FuncTy = FunctionType::get(
      /*Result=*/Type::getVoidTy(IContext::getContextRef()),
      /*Params=*/FuncTy_args,
      /*isVarArg=*/false);

  PointerType* PointerTy = PointerType::get(FuncTy, 0);

  Function* function = IContext::Mod->getFunction(function_name);
  if (!function) {
    function = Function::Create(
        /*Type=*/FuncTy,
        /*Linkage=*/GlobalValue::ExternalLinkage,
        /*Name=*/function_name, IContext::Mod);
    function->setCallingConv(67);
  }
  AttributeSet function_PAL;
  {
    SmallVector<AttributeSet, 4> Attrs;
    AttributeSet PAS;
    {
      AttrBuilder B;
      B.addAttribute(Attribute::NoUnwind);
      PAS = AttributeSet::get(IContext::getContextRef(), ~0U, B);
    }

    Attrs.push_back(PAS);
    function_PAL = AttributeSet::get(IContext::getContextRef(), Attrs);
  }

  // Constant Definitions
  Constant* fct_ptr =
      ConstantExpr::getCast(Instruction::BitCast, function, PointerTy);

  return fct_ptr;
}

Constant* GetIntFunctionPointer(StringRef function_name) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  // Type Definitions
  std::vector<Type*> FuncTy_args;
  FuncTy_args.push_back(IntegerType::get(IContext::getContextRef(), 32));
  FunctionType* FuncTy = FunctionType::get(
      /*Result=*/Type::getVoidTy(IContext::getContextRef()),
      /*Params=*/FuncTy_args,
      /*isVarArg=*/false);

  PointerType* PointerTy = PointerType::get(FuncTy, 0);

  Function* function = IContext::Mod->getFunction(function_name);
  if (!function) {
    function = Function::Create(
        /*Type=*/FuncTy,
        /*Linkage=*/GlobalValue::ExternalLinkage,
        /*Name=*/function_name, IContext::Mod);
    function->setCallingConv(67);
  }
  AttributeSet function_PAL;
  {
    SmallVector<AttributeSet, 4> Attrs;
    AttributeSet PAS;
    {
      AttrBuilder B;
      B.addAttribute(Attribute::NoUnwind);
      PAS = AttributeSet::get(IContext::getContextRef(), ~0U, B);
    }

    Attrs.push_back(PAS);
    function_PAL = AttributeSet::get(IContext::getContextRef(), Attrs);
  }

  // Constant Definitions
  Constant* fct_ptr =
      ConstantExpr::getCast(Instruction::BitCast, function, PointerTy);

  return fct_ptr;
}

void CreateCall(SDNode* N, IRBuilder<>* IRB, uint32_t Tgt) {
  if (IContext::Mod == NULL) inception_error("API has not been initialized.");

  // TODO: Look up address in symbol table.
  std::string FName =
      IContext::alm->Dec->getDisassembler()->syms->getFunctionName(Tgt);

  Module* Mod = IContext::Mod;

  Function* Func = Mod->getFunction(FName);
  if (Func == NULL) {
    Constant* func_ptr = GetVoidFunctionPointer(FName);
    Func = cast<Function>(func_ptr);
    // inception_error("BranchLifter cannot resolve address : 0x%08x", Tgt);
  }

  // if (Func->isIntrinsic()) {
  //   return;
  // }

  if (Func->empty()) {
    AttributeSet AS;
    AS = AS.addAttribute(IContext::getContextRef(), AttributeSet::FunctionIndex,
                         "DecompileLater", "True");
    Func->setAttributes(AS);
  }

  Value* Call;

  ABIAdapter abi_higher = ABIAdapter();

  if ((Call = abi_higher.Higher(Func, IRB)) == NULL) {
    inception_warning(
        "Function %s has unsupported parameter type... Call to this function \n"
        "aborted. This means that if the fail occurs during ICP creation, \n"
        "Analyzer may fail with unsupported indirect call to a valid entry \n"
        "point. If, the fail occured during the decompilation process please \n"
        "concider this code has wrong !\n\n\n",
        FName.c_str());
    return;
  }

  ABIAdapter abi_lower = ABIAdapter();

  if (abi_lower.Lower(Call, IRB) == NULL) {
    inception_warning(
        "Function %s has unsupported return type... Possible corrution because \n"
        "instruction may have been generated to lower arguments before \n"
        "unsupported type was encountered...\n\n\n",
        FName.c_str());
  }

  if (N != NULL) {
    Value* dummyLR = getConstant("0");
    saveNodeValue(N, dummyLR);
  }
  return;

  // TODO: Technically visitCall sets the LR to IP+8. We should return that.
}
