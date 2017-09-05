#include "Target/ARM/BranchLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include <vector>

#include "Utils/Builder.h"
#include "Utils/IContext.h"

using namespace llvm;
using namespace fracture;

void BranchLifter::registerLifter() {
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tBX_RET,
                      (LifterHandler)&BranchLifter::BranchHandler);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tB,
                      (LifterHandler)&BranchLifter::BranchHandlerB);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tBcc,
                      (LifterHandler)&BranchLifter::BranchHandlerB);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tBL,
                      (LifterHandler)&BranchLifter::BranchHandlerBL);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::t2B,
                      (LifterHandler)&BranchLifter::BranchHandlerB);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::t2Bcc,
                      (LifterHandler)&BranchLifter::BranchHandlerB);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tBLXr,
                      (LifterHandler)&BranchLifter::BranchHandlerBLXr);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tCBZ,
                      (LifterHandler)&BranchLifter::BranchHandlerCB);
  alm->registerLifter(this, std::string("BranchLifter"), (unsigned)ARM::tCBNZ,
                      (LifterHandler)&BranchLifter::BranchHandlerCB);
}

void BranchLifter::BranchHandler(SDNode *N, IRBuilder<> *IRB) {
  Instruction *Ret = IRB->CreateRetVoid();
  Ret->setDebugLoc(N->getDebugLoc());
  saveNodeValue(N, Ret);
  return;
}

void BranchLifter::BranchHandlerB(SDNode *N, IRBuilder<> *IRB) {
  // Get the address
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(1));
  if (!DestNode) {
    outs() << "visitBRCOND: Not a constant integer for branch!\n";
    return;
  }

  uint32_t DestInt = DestNode->getSExtValue();
  uint32_t PC = alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  // Note: pipeline is 8 bytes
  uint32_t Tgt = PC + 4 + DestInt;

  Function *F = IRB->GetInsertBlock()->getParent();
  BasicBlock *CurBB = IRB->GetInsertBlock();

  BasicBlock *BBTgt = alm->Dec->getOrCreateBasicBlock(Tgt, F);

  // Parse the branch condition code
  const ConstantSDNode *CCNode = dyn_cast<ConstantSDNode>(N->getOperand(2));
  if (!CCNode) {
    errs() << "visitBRCOND: Condition code is not a constant integer!\n";
    return;
  }

  // create the code that check the condition
  // NULL if condition is AL
  int cond = CCNode->getZExtValue();
  Value *Cmp = createCondition(cond, IRB);

  // Unconditional branch or inside it block (last instruction)
  if (Cmp == NULL || (IContext::alm->Dec->it_state & 0b1111) != 0) {
    Instruction *Br = IRB->CreateBr(BBTgt);
    Br->setDebugLoc(N->getDebugLoc());
    saveNodeValue(N, Br);
    return;
  }

  // If conditional branch, find the successor block and look at CC
  BasicBlock *NextBB = NULL;
  Function::iterator BI = F->begin(), BE = F->end();
  while (BI != BE && BI->getName() != CurBB->getName()) ++BI;
  ++BI;
  if (BI == BE) {  // NOTE: This should never happen...
    NextBB = alm->Dec->getOrCreateBasicBlock("end", F);
  } else {
    NextBB = &(*BI);
  }

  // Conditional branch
  Instruction *Br = IRB->CreateCondBr(Cmp, BBTgt, NextBB);
  Br->setDebugLoc(N->getDebugLoc());
  saveNodeValue(N, Br);
  return;
}

// TODO: handle conditions in factorized code for all branch types
// This version of BL does not handle conditions
void BranchLifter::BranchHandlerBL(SDNode *N, IRBuilder<> *IRB) {
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(3));
  if (!DestNode) {
    outs() << "visitCALL: Not a constant integer for call!";
    return;
  }

  int64_t DestInt = DestNode->getSExtValue();
  int64_t PC = alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  int64_t Tgt = PC + 4 + DestInt;
  CallHandler(N, IRB, Tgt);
}

// simple indirect call promotion
void BranchLifter::BranchHandlerBLXr(SDNode *N, IRBuilder<> *IRB) {
  // pointer register
  Value *Rm = visit(N->getOperand(3).getNode(), IRB);
  Rm = IRB->CreateAnd(Rm, getConstant("4294967294"));  // remove last bit

  // if necessary create the indirect call promotion function
  Function *icp = IContext::Mod->getFunction("icp");
  if (icp == NULL) {
    Constant *c = IContext::Mod->getOrInsertFunction(
        "icp", Type::getVoidTy(IContext::getContextRef()),
        IntegerType::get(IContext::getContextRef(), 32), NULL);
    icp = cast<Function>(c);
    icp->setCallingConv(CallingConv::C);
    Function::arg_iterator args = icp->arg_begin();
    Value *ptr_reg = args;
    ptr_reg->setName("ptr_reg");

    // for each function (address/name) in the symbol table
    object::ObjectFile *Executable =
        alm->Dec->getDisassembler()->getExecutable();
    uint64_t SymAddr;
    std::error_code ec;
    StringRef NameRef;

    IRBuilder<> *bbIRB = NULL;

    BasicBlock *entry_block =
        BasicBlock::Create(IContext::getContextRef(), "entry", icp);

    unsigned num_cases = 0;
    std::vector<ConstantInt *> addresses;
    std::vector<BasicBlock *> blocks;
    for (object::symbol_iterator I = Executable->symbols().begin(),
                                 E = Executable->symbols().end();
         I != E; ++I) {
      object::SymbolRef::Type SymbolTy;
      if ((ec = I->getType(SymbolTy))) {
        errs() << ec.message() << "\n";
        continue;
      }
      if (SymbolTy != object::SymbolRef::ST_Function) {
        continue;
      }
      if ((ec = I->getAddress(SymAddr))) {
        errs() << ec.message() << "\n";
        continue;
      }
      if ((ec = I->getName(NameRef))) {
        errs() << ec.message() << "\n";
        continue;
      }

      if (NameRef.empty()) {
        std::string *FName = new std::string();
        raw_string_ostream FOut(*FName);
        FOut << "func_" << format("%1" PRIx64, (unsigned)SymAddr);
        NameRef = StringRef(FOut.str());
      }
      // if the register points to the function, call it
      // outs() << "FUNCTION " << NameRef << " " << (unsigned)SymAddr << "\n";
      BasicBlock *bb = BasicBlock::Create(IContext::getContextRef(),
                                          "call_" + NameRef.str(), icp);
      bbIRB = new IRBuilder<>(bb);
      CallHandler(NULL, bbIRB, (unsigned)SymAddr);
      bbIRB->CreateRetVoid();
      delete bbIRB;
      num_cases++;
      // Value *cmp = prevIRB->CreateICmpEQ(
      //    ptr_reg, ConstantInt::get(IContext::getContextRef(),
      //                              APInt(32, (unsigned)SymAddr, 10)));
      SymAddr &= 0xfffffffe;  // remove last bit
      ConstantInt *addr = ConstantInt::get(IContext::getContextRef(),
                                           APInt(32, (unsigned)SymAddr, 10));
      addresses.push_back(addr);
      blocks.push_back(bb);
    }

    BasicBlock *end_block =
        BasicBlock::Create(IContext::getContextRef(), "end", icp);
    bbIRB = new IRBuilder<>(end_block);
    bbIRB->CreateRetVoid();
    delete bbIRB;

    IRBuilder<> *entryIRB = new IRBuilder<>(entry_block);
    SwitchInst *sw = entryIRB->CreateSwitch(ptr_reg, end_block, num_cases);

    for (unsigned int i = 0; i < num_cases; i++) {
      sw->addCase(addresses[i], blocks[i]);
    }
  }

  IRB->CreateCall(icp, Rm);
  Value *dummyLR = getConstant("0");
  saveNodeValue(N, dummyLR);
}

void BranchLifter::CallHandler(SDNode *N, IRBuilder<> *IRB, uint32_t Tgt) {
  // TODO: Look up address in symbol table.
  std::string FName = alm->Dec->getDisassembler()->getFunctionName(Tgt);

  Module *Mod = IRB->GetInsertBlock()->getParent()->getParent();

  Function *Func = Mod->getFunction(FName);
  if (Func == NULL) {
    errs() << "[BranchLifter] Unable to resolve address : " << Tgt << "\n";
    exit(0);
  }

  // outs() << FName << " args:\n";
  std::vector<Value *> Args;
  std::vector<Type *> ArgTypes;
  char reg_name[3] = "R0";
  for (Function::arg_iterator I = Func->arg_begin(), E = Func->arg_end();
       I != E; ++I) {
    ArgTypes.push_back(I->getType());
    if (I->getType()->isPointerTy()) {
      Value *Res = ReadReg(Reg(reg_name), IRB);
      Res = IRB->CreateIntToPtr(Res, I->getType());
      Args.push_back(Res);
      reg_name[1]++;
    } else if (I->getType()->isIntegerTy()) {
      Value *Res = ReadReg(Reg(reg_name), IRB);
      Res = IRB->CreateTrunc(
          Res, IntegerType::get(IContext::getContextRef(),
                                I->getType()->getIntegerBitWidth()));
      Args.push_back(Res);
      reg_name[1]++;
    } else if (I->getType()->isArrayTy()) {
      Type *Ty = ArrayType::get(
          IntegerType::get(IContext::getContextRef(), 32),
          I->getType()->getArrayNumElements());
      Value *array = IRB->CreateAlloca(Ty);
      for (unsigned int i = 0; i < I->getType()->getArrayNumElements(); i++) {
        Value *IdxList[2];
        IdxList[0] = getConstant("0");
        IdxList[1] = ConstantInt::get(IContext::getContextRef(),
                                      APInt(32, i, 10));
        Value *ptr = IRB->CreateGEP(array, IdxList);
        IRB->CreateStore(ReadReg(Reg(reg_name), IRB), ptr);
        reg_name[1]++;
      }
      Value *Res = IRB->CreateLoad(array);
      Args.push_back(Res);
    } else {
      Args.push_back(ReadReg(Reg(reg_name), IRB));
      reg_name[1]++;
    }
  }

  FunctionType *FT = FunctionType::get(Func->getReturnType(), ArgTypes, false);

  Twine TgtAddr(Tgt);

  // outs() << " =========================== \n\n";
  // outs() << "Tgt        :  " << format("%8" PRIx64, Tgt) << '\n';
  // outs() << "instrSize  :  " << format("%8" PRIx64, 4) << '\n';
  // outs() << "FName      :  " << FName << '\n';
  // outs() << " =========================== \n\n";

  AttributeSet AS;
  AS = AS.addAttribute(IContext::getContextRef(),
                       AttributeSet::FunctionIndex, "Address", TgtAddr.str());

  Function *Proto = cast<Function>(Mod->getOrInsertFunction(FName, FT, AS));

  Proto->setCallingConv(Func->getCallingConv());
  Value *Call = IRB->CreateCall(dyn_cast<Value>(Proto), Args);
  if (!Func->getReturnType()->isVoidTy()) {
    if (Func->getReturnType()->isPointerTy()) {
      Call = IRB->CreatePtrToInt(
          Call, IntegerType::get(IContext::getContextRef(), 32));
    } else if (Func->getReturnType()->isIntegerTy()) {
      Call = IRB->CreateZExt(
          Call, IntegerType::get(IContext::getContextRef(), 32));
    }

    WriteReg(Call, Reg("R0"), IRB);
  }

  if (N != NULL) {
    Value *dummyLR = getConstant("0");
    saveNodeValue(N, dummyLR);
  }
  return;

  // TODO: Technically visitCall sets the LR to IP+8. We should return that.
}

void BranchLifter::BranchHandlerCB(SDNode *N, IRBuilder<> *IRB) {
  // Get the address
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(2));
  if (!DestNode) {
    outs() << "visitBRCOND: Not a constant integer for branch!\n";
    return;
  }

  uint32_t DestInt = DestNode->getSExtValue();
  uint32_t PC = alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  // Note: pipeline is 8 bytes
  uint32_t Tgt = PC + 4 + DestInt;

  Function *F = IRB->GetInsertBlock()->getParent();
  BasicBlock *CurBB = IRB->GetInsertBlock();

  BasicBlock *BBTgt = alm->Dec->getOrCreateBasicBlock(Tgt, F);

  // get the condition register
  Value *Rn = visit(N->getOperand(1).getNode(), IRB);

  // find the successor block
  BasicBlock *NextBB = NULL;
  Function::iterator BI = F->begin(), BE = F->end();
  while (BI != BE && BI->getName() != CurBB->getName()) ++BI;
  ++BI;
  if (BI == BE) {  // NOTE: This should never happen...
    NextBB = alm->Dec->getOrCreateBasicBlock("end", F);
  } else {
    NextBB = &(*BI);
  }

  // Compute the condition
  Value *Cmp = NULL;
  switch (N->getMachineOpcode()) {
    default:
      errs() << "[BranchHandlerCB] Error, unknown opcode\n";
      return;
    case ARM::tCBZ:
      Cmp = IRB->CreateICmpEQ(Rn, getConstant("0"));
      break;
    case ARM::tCBNZ:
      Cmp = IRB->CreateICmpNE(Rn, getConstant("0"));
      break;
  }

  // Conditional branch
  Instruction *Br = IRB->CreateCondBr(Cmp, BBTgt, NextBB);
  Br->setDebugLoc(N->getDebugLoc());
  saveNodeValue(N, Br);
  return;
}
