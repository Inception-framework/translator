
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
#include "Target/ARM/FlagsLifter.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>

using namespace llvm;

void FlagsLifter::WriteZF(IRBuilder<> *IRB, llvm::Value *w) {
  // set ZF
  // ZF is set if the result is 0 and clear otherwise
  auto *icmp = IRB->CreateICmp(llvm::CmpInst::ICMP_EQ, w, getConstant("0"));

  icmp = Bool2Int(icmp, IRB);

  WriteReg(icmp, Reg("ZF"), IRB);
}

void FlagsLifter::WriteCFShiftR(IRBuilder<> *IRB, llvm::Value *shift_min_1) {
  auto lsb = IRB->CreateAnd(shift_min_1, getConstant("1"));

  lsb = Bool2Int(lsb, IRB);

  WriteReg(lsb, Reg("CF"), IRB);
}

void FlagsLifter::WriteCFShiftL(IRBuilder<> *IRB, llvm::Value *shift_min_1,
                                llvm::Value *isShift) {
  auto msb = IRB->CreateLShr(shift_min_1, getConstant("31"));

  auto trunc = Bool2Int(msb, IRB);

  // keep old cf if no shift
  auto update = IRB->CreateAnd(trunc, isShift);
  auto old = IRB->CreateAnd(ReadReg(Reg("CF"), IRB), IRB->CreateNot(isShift));
  auto cf = IRB->CreateOr(update, old);
  WriteReg(cf, Reg("CF"), IRB);
}

void FlagsLifter::WriteCFAdc(IRBuilder<> *IRB, llvm::Value *res,
                             llvm::Value *argL) {
  auto cmpRes = IRB->CreateICmp(llvm::CmpInst::ICMP_ULT, res, argL);

  cmpRes = Bool2Int(cmpRes, IRB);

  auto cf = ReadReg(Reg("CF"), IRB);

  auto xor1 = IRB->CreateOr(cmpRes, cf);

  WriteReg(xor1, Reg("CF"), IRB);
}

void FlagsLifter::WriteCFAdd(IRBuilder<> *IRB, llvm::Value *res,
                             llvm::Value *argL) {
  // cf = res < argL
  auto cmpRes = IRB->CreateICmp(llvm::CmpInst::ICMP_ULT, res, argL);

  cmpRes = Bool2Int(cmpRes, IRB);

  WriteReg(cmpRes, Reg("CF"), IRB);
}

void FlagsLifter::WriteCFconstant(IRBuilder<> *IRB, uint32_t constant) {
  uint32_t byte0 = constant & 0x000000ff;
  uint32_t byte1 = (constant >> 8) & 0x000000ff;
  uint32_t byte2 = (constant >> 16) & 0x000000ff;
  uint32_t byte3 = (constant >> 24) & 0x000000ff;
  if (!(byte3 == byte1 && byte0 == 0 && byte2 == 0) &&
      !(byte0 == byte1 && byte0 == byte2 && byte0 == byte3) &&
      !(byte3 == 0 && byte2 == 0 && byte1 == 0) &&
      !(byte2 == byte0 && byte3 == 0 && byte1 == 0)) {
    if (byte3 & 0x80) {
      WriteReg(getConstant("1"), Reg("CF"), IRB);
    } else {
      WriteReg(getConstant("0"), Reg("CF"), IRB);
    }
  }
}

void FlagsLifter::WriteVFSub(IRBuilder<> *IRB, llvm::Value *res,
                             llvm::Value *lhs, llvm::Value *rhs) {
  // of = lshift((lhs ^ rhs ) & (lhs ^ res), 12 - width) & 2048
  // where lshift is written as if n >= 0, x << n, else x >> (-n)
  auto xor1 = IRB->CreateXor(lhs, rhs);
  auto xor2 = IRB->CreateXor(lhs, res);
  auto anded = IRB->CreateAnd(xor1, xor2);

  llvm::Value *shifted = nullptr;
  // extract sign bit
  shifted = IRB->CreateLShr(anded, getConstant("31"));

  // truncate anded1
  auto trunced = Bool2Int(shifted, IRB);

  // write to OF
  WriteReg(trunced, Reg("VF"), IRB);
}

void FlagsLifter::WriteVFAdd(IRBuilder<> *IRB, llvm::Value *res,
                             llvm::Value *lhs, llvm::Value *rhs) {
  // of = lshift((lhs ^ rhs ^ -1) & (lhs ^ res), 12 - width) & 2048
  // where lshift is written as if n >= 0, x << n, else x >> (-n)

  auto xor1 = IRB->CreateXor(lhs, rhs);
  auto xor2 = IRB->CreateXor(xor1, getConstant("-1"));
  auto xor3 = IRB->CreateXor(lhs, res);
  auto anded = IRB->CreateAnd(xor2, xor3);

  llvm::Value *shifted = nullptr;
  // shifts corrected to always place the OF bit
  // in the bit 0 position. This way it works for
  // all sized ints
  shifted = IRB->CreateLShr(anded, getConstant("31"));

  // and by 1
  auto anded1 = IRB->CreateAnd(shifted, getConstant("1"));

  // truncate anded1
  auto trunced = Bool2Int(anded1, IRB);

  // write to OF
  WriteReg(trunced, Reg("VF"), IRB);
}

void FlagsLifter::WriteVFAdc(IRBuilder<> *IRB, llvm::Value *res,
                             llvm::Value *lhs, llvm::Value *rhs) {
  // of = lshift((lhs ^ rhs ^ -1) & (lhs ^ res), 12 - width) & 2048
  // where lshift is written as if n >= 0, x << n, else x >> (-n)

  auto xor1 = IRB->CreateXor(lhs, rhs);
  auto xor2 = IRB->CreateXor(xor1, getConstant("-1"));
  auto xor3 = IRB->CreateXor(lhs, res);
  auto anded = IRB->CreateAnd(xor2, xor3);

  llvm::Value *shifted = nullptr;
  // shifts corrected to always place the OF bit
  // in the bit 0 position. This way it works for
  // all sized ints
  shifted = IRB->CreateLShr(anded, getConstant("31"));

  // and by 1
  auto vf2 = IRB->CreateAnd(shifted, getConstant("1"));

  // total ovf = (ovf1 + ovf2) mod 2
  auto vf1 = ReadReg(Reg("VF"), IRB);
  auto add = IRB->CreateAdd(vf1, vf2);
  auto anded1 = IRB->CreateAnd(add, getConstant("1"));

  // truncate anded1
  auto trunced = Bool2Int(anded1, IRB);

  WriteReg(trunced, Reg("VF"), IRB);
}

void FlagsLifter::WriteNF(IRBuilder<> *IRB, llvm::Value *written) {
  //%1 = SIGNED CMP %written < 0
  // Value   *scmp = new ICmpInst(   *b,
  //                                ICmpInst::ICMP_SLT,
  //                                written,
  //                                CONST_V<width>(b, 0));

  // extract sign bit
  auto signBit = IRB->CreateLShr(written, getConstant("31"));

  auto trunc = Bool2Int(signBit, IRB);

  WriteReg(trunc, Reg("NF"), IRB);
}

