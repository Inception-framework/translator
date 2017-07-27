/*
 Copyright (c) 2014, Trail of Bits
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice, this
 list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 Neither the name of Trail of Bits nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "Target/ARM/FlagsLifter.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>

using namespace llvm;

// void FlagsLifter::WriteNFAdd(IRBuilder<> *IRB, llvm::Value *res) {
//  auto shifted = IRB->CreateLShr(res, getConstant("31"));
//
//  auto *icmp =
//      IRB->CreateICmp(llvm::CmpInst::ICMP_EQ, shifted, getConstant("1"));
//
//  // truncate anded1
//  auto trunced = Bool2Int(icmp, IRB);
//
//  // write to NF
//  WriteReg(trunced, Reg("NF"), NULL, IRB);
//}

void FlagsLifter::WriteZF(IRBuilder<> *IRB, llvm::Value *w) {
  // set ZF
  // ZF is set if the result is 0 and clear otherwise
  auto *icmp = IRB->CreateICmp(llvm::CmpInst::ICMP_EQ, w, getConstant("0"));

  icmp = Bool2Int(icmp, IRB);

  WriteReg(icmp, Reg("ZF"), NULL, IRB);
}

void FlagsLifter::WriteCFShiftR(IRBuilder<> *IRB, llvm::Value *val,
                                llvm::Value *shiftAmt) {
  auto n = IRB->CreateSub(shiftAmt, getConstant("1"));
  auto v = IRB->CreateLShr(val, n);

  v = Bool2Int(v, IRB);

  WriteReg(v, Reg("CF"), NULL, IRB);
}

void FlagsLifter::WriteCFAdc(IRBuilder<> *IRB, llvm::Value *res,
                             llvm::Value *argL) {
  auto cmpRes = IRB->CreateICmp(llvm::CmpInst::ICMP_ULT, res, argL);

  cmpRes = Bool2Int(cmpRes, IRB);

  auto cf = ReadReg(Reg("CF"), IRB);

  auto xor1 = IRB->CreateOr(cmpRes, cf);

  WriteReg(xor1, Reg("CF"), NULL, IRB);
}

void FlagsLifter::WriteCFAdd(IRBuilder<> *IRB, llvm::Value *res,
                             llvm::Value *argL) {
  // cf = res < argL
  auto cmpRes = IRB->CreateICmp(llvm::CmpInst::ICMP_ULT, res, argL);

  cmpRes = Bool2Int(cmpRes, IRB);

  WriteReg(cmpRes, Reg("CF"), NULL, IRB);
}

void FlagsLifter::WriteCFSub(IRBuilder<> *IRB, llvm::Value *argL,
                             llvm::Value *argR) {
  auto cmpRes = IRB->CreateICmp(llvm::CmpInst::ICMP_ULT, argL, argR);

  cmpRes = Bool2Int(cmpRes, IRB);

  WriteReg(cmpRes, Reg("CF"), NULL, IRB);
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
  WriteReg(trunced, Reg("VF"), NULL, IRB);
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
  WriteReg(trunced, Reg("VF"), NULL, IRB);
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

  WriteReg(trunced, Reg("VF"), NULL, IRB);
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

  WriteReg(trunc, Reg("NF"), NULL, IRB);
}
