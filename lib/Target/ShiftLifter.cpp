
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

#include "Target/ARM/ShiftLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"

using namespace llvm;
using namespace fracture;

void ShiftLifter::registerLifter() {
  // LSL rd,rm,<rs|sh>
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::tLSLri,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSL);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::tLSLrr,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSL);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2LSLri,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSL);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2LSLrr,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSL);
  // LSR rd,rm,<rs|sh>
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::tLSRri,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSR);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::tLSRrr,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSR);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2LSRri,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSR);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2LSRrr,
                      (LifterHandler)&ShiftLifter::ShiftHandlerLSR);
  // ASR rd,rm,<rs|sh>
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::tASRri,
                      (LifterHandler)&ShiftLifter::ShiftHandlerASR);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::tASRrr,
                      (LifterHandler)&ShiftLifter::ShiftHandlerASR);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2ASRri,
                      (LifterHandler)&ShiftLifter::ShiftHandlerASR);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2ASRrr,
                      (LifterHandler)&ShiftLifter::ShiftHandlerASR);
  // ROR rd,rm,<rs|sh>
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::tROR,
                      (LifterHandler)&ShiftLifter::ShiftHandlerROR);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2RORri,
                      (LifterHandler)&ShiftLifter::ShiftHandlerROR);
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2RORrr,
                      (LifterHandler)&ShiftLifter::ShiftHandlerROR);
  // RRX rd,rm
  alm->registerLifter(this, std::string("ShiftLifter"), (unsigned)ARM::t2RRX,
                      (LifterHandler)&ShiftLifter::ShiftHandlerRRX);
}

void ShiftLifter::ShiftHandlerShiftOp(SDNode *N, IRBuilder<> *IRB) {
  unsigned opcode = N->getMachineOpcode();
  int index = -1;
  switch (opcode) {
    case ARM::t2TSTrs:
    case ARM::t2TEQrs:
    case ARM::t2ANDrs:
    case ARM::t2EORrs:
    case ARM::t2ORRrs:
    case ARM::t2ORNrs:
    case ARM::t2BICrs:
    case ARM::t2ADDSrs:
    case ARM::t2ADDrs:
    case ARM::t2ADCrs:
    case ARM::t2SUBSrs:
    case ARM::t2SUBrs:
    case ARM::t2SBCrs:
    case ARM::t2CMPrs:
    case ARM::t2CMNzrs:
      index = 2;
      break;
    case ARM::t2MVNs:
      index = 1;
      break;
    default:
      outs() << "not supported\n";
      return;
  }
  const ConstantSDNode *ConstNode =
      dyn_cast<ConstantSDNode>(N->getOperand(index));
  if (!ConstNode) {
    outs() << "TstHandler: Not a constant integer for immediate!\n";
    return;
  }
  uint32_t shift_t_n = ConstNode->getZExtValue();
  uint32_t shift_t = shift_t_n & 0x7;

  switch (shift_t) {
    case 0x1:
      ShiftHandlerASR(N, IRB);
      break;
    case 0x2:
      ShiftHandlerLSL(N, IRB);
      break;
    case 0x3:
      ShiftHandlerLSR(N, IRB);
      break;
    case 0x4:
      ShiftHandlerROR(N, IRB);
      break;
    case 0x5:
      ShiftHandlerRRX(N, IRB);
      break;
    default:
      outs() << "No valid shift type in shift operand\n";
      return;
  }
}

void ShiftLifter::ShiftHandlerLSL(SDNode *N, IRBuilder<> *IRB) {
  ARMSHIFTInfo *info = RetrieveGraphInformation(N, IRB);

  Value *shift = IRB->CreateICmpNE(info->Op1, getConstant("0"));
  shift = Bool2Int(shift, IRB);

  Value *shift_amount_min1 = IRB->CreateSub(info->Op1, shift);

  Value *partial_res = IRB->CreateShl(info->Op0, shift_amount_min1);

  Instruction *Res = dyn_cast<Instruction>(IRB->CreateShl(partial_res, shift));

  if (info->S) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    ////Compute NF
    // flags->WriteNFAdd(IRB, Res);
    // Compute NF.
    flags->WriteNF(IRB, Res);
    // Compute ZF.
    flags->WriteZF(IRB, Res);
    // Compute CF.
    flags->WriteCFShiftL(IRB, partial_res, shift);
  }

  Res->setDebugLoc(N->getDebugLoc());

  saveNodeValue(N, Res);
}

// note1: shift in two parts, because llvm does not allow shifting a 32-bit
// value
// by 32 bit
// note2: the range is 1-32. In case of immediate, 32 is encoded as 0.
void ShiftLifter::ShiftHandlerLSR(SDNode *N, IRBuilder<> *IRB) {
  ARMSHIFTInfo *info = RetrieveGraphInformation(N, IRB);

  Value *const_31 = getConstant("31");

  Value *const_1 = getConstant("1");

  Value *const_0 = getConstant("0");

  Value *shift_amount_min1;
  if (info->Op1 == const_0) {
    shift_amount_min1 = IRB->CreateAdd(info->Op1, const_31);
  } else {
    shift_amount_min1 = IRB->CreateSub(info->Op1, const_1);
  }

  Value *partial_res;
  partial_res = IRB->CreateLShr(info->Op0, shift_amount_min1);

  Instruction *Res =
      dyn_cast<Instruction>(IRB->CreateLShr(partial_res, const_1));

  if (info->S) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    ////Compute NF
    // flags->WriteNFAdd(IRB, Res);
    // Compute NF.
    flags->WriteNF(IRB, Res);
    // Compute ZF.
    flags->WriteZF(IRB, Res);
    // Compute CF.
    flags->WriteCFShiftR(IRB, partial_res);
  }

  Res->setDebugLoc(N->getDebugLoc());

  saveNodeValue(N, Res);
}

// note1: shift in two parts, because llvm does not allow shifting a 32-bit
// value
// by 32 bit
// note2: the range is 1-32. In case of immediate, 32 is encoded as 0.
void ShiftLifter::ShiftHandlerASR(SDNode *N, IRBuilder<> *IRB) {
  ARMSHIFTInfo *info = RetrieveGraphInformation(N, IRB);

  Value *const_31 = getConstant("31");

  Value *const_1 = getConstant("1");

  Value *const_0 = getConstant("0");

  Value *shift_amount_min1;
  // StringRef BaseName = getBaseValueName(shift_amount_min1->getName());
  // StringRef Name = getIndexedValueName(BaseName);
  if (info->Op1 == const_0) {
    shift_amount_min1 = IRB->CreateAdd(info->Op1, const_31);
  } else {
    shift_amount_min1 = IRB->CreateSub(info->Op1, const_1);
  }

  Value *partial_res;
  // BaseName = getBaseValueName(partial_res->getName());
  // Name = getIndexedValueName(BaseName);
  partial_res = IRB->CreateAShr(info->Op0, shift_amount_min1);

  Instruction *Res =
      dyn_cast<Instruction>(IRB->CreateAShr(partial_res, const_1));

  if (info->S) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    ////Compute NF
    // flags->WriteNFAdd(IRB, Res);
    // Compute NF.
    flags->WriteNF(IRB, Res);
    // Compute ZF.
    flags->WriteZF(IRB, Res);
    // Compute CF.
    flags->WriteCFShiftR(IRB, partial_res);
  }

  Res->setDebugLoc(N->getDebugLoc());

  saveNodeValue(N, Res);
}

// Note: llvm does not have ror
void ShiftLifter::ShiftHandlerROR(SDNode *N, IRBuilder<> *IRB) {
  ARMSHIFTInfo *info = RetrieveGraphInformation(N, IRB);

  Value *const_32 = getConstant("32");

  Value *lshift_amount = IRB->CreateSub(const_32, info->Op1);
  Value *high = IRB->CreateShl(info->Op0, lshift_amount);
  Value *low = IRB->CreateLShr(info->Op0, info->Op1);

  Instruction *Res = dyn_cast<Instruction>(IRB->CreateOr(high, low));

  if (info->S) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    ////Compute NF
    // flags->WriteNFAdd(IRB, Res);
    // Compute NF.
    flags->WriteNF(IRB, Res);
    // Compute ZF.
    flags->WriteZF(IRB, Res);
    // Compute CF. Reuse ShiftL code but with final result
    flags->WriteCFShiftL(IRB, Res, getConstant("1"));
  }

  Res->setDebugLoc(N->getDebugLoc());

  saveNodeValue(N, Res);
}

void ShiftLifter::ShiftHandlerRRX(SDNode *N, IRBuilder<> *IRB) {
  ARMSHIFTInfo *info = RetrieveGraphInformation(N, IRB);

  Value *cf_in = ReadReg(Reg("CF"), IRB);
  Value *msb = IRB->CreateShl(cf_in, getConstant("31"));
  Value *rshift1 = IRB->CreateLShr(info->Op0, getConstant("1"));

  Instruction *Res = dyn_cast<Instruction>(IRB->CreateOr(msb, rshift1));

  if (info->S) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter *flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    ////Compute NF
    // flags->WriteNFAdd(IRB, Res);
    // Compute NF.
    flags->WriteNF(IRB, Res);
    // Compute ZF.
    flags->WriteZF(IRB, Res);
    // Compute CF. Reuse ShiftR code but with initial value
    flags->WriteCFShiftR(IRB, info->Op0);
  }

  Res->setDebugLoc(N->getDebugLoc());

  saveNodeValue(N, Res);
}

ARMSHIFTInfo *ShiftLifter::RetrieveGraphInformation(SDNode *N,
                                                    IRBuilder<> *IRB) {
  Value *Op0 = NULL;
  Value *Op1 = NULL;
  bool S = false;

  uint32_t shift_t_n = -1, shift_n = -1;
  const ConstantSDNode *ConstNode = NULL;

  unsigned opcode = N->getMachineOpcode();
  switch (opcode) {
    // instructions which have an operand that has to be shifted
    case ARM::t2ANDrs:
    case ARM::t2EORrs:
    case ARM::t2ORRrs:
    case ARM::t2ORNrs:
    case ARM::t2BICrs:
    case ARM::t2ADDSrs:
    case ARM::t2ADDrs:
    case ARM::t2ADCrs:
    case ARM::t2SUBSrs:
    case ARM::t2SUBrs:
    case ARM::t2SBCrs:
      Op0 = visit(N->getOperand(1).getNode(), IRB);
      ConstNode = dyn_cast<ConstantSDNode>(N->getOperand(2));
      if (!ConstNode) {
        outs() << "TstHandler: Not a constant integer for immediate!\n";
        exit(1);
      }
      shift_t_n = ConstNode->getZExtValue();
      shift_n = shift_t_n >> 3;
      Op1 = ConstantInt::get(IContext::getContextRef(),
                             APInt(32, shift_n, 10));
      S = IsSetFlags(N);
      break;
    case ARM::t2TSTrs:
    case ARM::t2TEQrs:
    case ARM::t2CMPrs:
    case ARM::t2CMNzrs:
      Op0 = visit(N->getOperand(1).getNode(), IRB);
      ConstNode = dyn_cast<ConstantSDNode>(N->getOperand(2));
      if (!ConstNode) {
        outs() << "TstHandler: Not a constant integer for immediate!\n";
        exit(1);
      }
      shift_t_n = ConstNode->getZExtValue();
      shift_n = shift_t_n >> 3;
      Op1 = ConstantInt::get(IContext::getContextRef(),
                             APInt(32, shift_n, 10));
      S = true;
      break;
    case ARM::t2MVNs:
      Op0 = visit(N->getOperand(0).getNode(), IRB);
      ConstNode = dyn_cast<ConstantSDNode>(N->getOperand(1));
      if (!ConstNode) {
        outs() << "MvnHandler: Not a constant integer for immediate!\n";
        exit(1);
      }
      shift_t_n = ConstNode->getZExtValue();
      shift_n = shift_t_n >> 3;
      Op1 = ConstantInt::get(IContext::getContextRef(),
                             APInt(32, shift_n, 10));
      S = IsSetFlags(N);
      break;
    default:
      // shift instructions
      Op0 = visit(N->getOperand(0).getNode(), IRB);
      Op1 = visit(N->getOperand(1).getNode(), IRB);
      S = IsSetFlags(N);
      break;
  }

  ARMSHIFTInfo *info = new ARMSHIFTInfo(Op0, Op1, S);

  return info;
}
