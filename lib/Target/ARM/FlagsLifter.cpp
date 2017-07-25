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

// template <int width>
// static void WriteZF(IRBuilder<> *IRB, llvm::Value *w) {
//   // set ZF
//   // ZF is set if the result is 0 and clear otherwise
//   Instruction *icmp =
//       IRB->CreateICmp(llvm::CmpInst::ICMP_EQ, w, CONST_V<width>(0));
//
//   WriteReg(icmp, Rd, CONST_V<width>(0), IRB);
// }
//
// template <int width>
// static void WriteCFShiftR(IRBuilder<> *IRB, llvm::Value *val,
//                           llvm::Value *shiftAmt) {
//   auto v = IRB->CreateLShr(
//       val,
//       IRB->CreateSub(shiftAmt, CONST_V<width>(b, 1), "", b),
//       "", b);
//   auto &C = b->getContext();
//   F_WRITE(b, llvm::X86::CF,
//           new llvm::ZExtInst(
//               new llvm::TruncInst(v, llvm::Type::getInt1Ty(C), "", b),
//               llvm::Type::getInt8Ty(C), "", b));
// }

void FlagsLifter::WriteAFAddSub(llvm::IRBuilder<> *IRB, llvm::Value *res,
                                llvm::Value *o1, llvm::Value *o2) {
  StringRef BaseNameO1 = getBaseValueName(o1->getName());
  StringRef Name;

  Name = getIndexedValueName(BaseNameO1);
  auto v1 = IRB->CreateXor(res, o1, Name);

  Name = getIndexedValueName(BaseNameO1);
  auto v2 = IRB->CreateXor(v1, o2, Name);

  Name = getIndexedValueName(BaseNameO1);
  auto v3 = IRB->CreateAnd(v2, getConstant("0"), Name);

  Name = getIndexedValueName(BaseNameO1);
  auto c =
      IRB->CreateICmp(llvm::CmpInst::ICMP_NE, v3, getConstant("0"), Name);

  Type* Ty = IntegerType::get(alm->Mod->getContext(), 32);

  Name = getIndexedValueName(BaseNameO1);

  auto &C = alm->Mod->getContext();

  auto bool_ty = llvm::Type::getInt1Ty(C);
  auto int32_ty = llvm::Type::getInt32Ty(C);

  if (c->getType() != bool_ty) {
    Name = getIndexedValueName(BaseNameO1);
    c = IRB->CreateTrunc(c, bool_ty, Name);
  }
  Name = getIndexedValueName(BaseNameO1);
  c = IRB->CreateZExt(c, int32_ty, Name);

  WriteReg(c, Reg("AF"), NULL, IRB);
}

// template <int width>
// static void WriteAF2(IRBuilder<> *IRB, llvm::Value *r, llvm::Value *lhs,
//                      llvm::Value *rhs) {
//   // this will implement the (r ^ lhs ^ rhs) & 0x10 approach used by VEX
//   // but it will also assume that the values as input are full width
//   auto t1 = IRB->CreateXor(r, lhs, "", b);
//   auto t2 = IRB->CreateXor(t1, rhs, "", b);
//   auto t3 = IRB->CreateAnd(t2, CONST_V<width>(b, 0x10), "",
//   b); auto cr =
//       new llvm::ICmpInst(*b, llvm::CmpInst::ICMP_NE, t3, CONST_V<width>(b,
//       0));
//   F_WRITE(b, llvm::X86::AF, cr);
// }
//
// template <int width>
// static void WriteCFAdd(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *argL)
// {
//   // cf = res < argL
//   auto cmpRes = new llvm::ICmpInst(*b, llvm::CmpInst::ICMP_ULT, res, argL);
//   F_WRITE(b, llvm::X86::CF, cmpRes);
// }
//
// static void WriteCFSub(IRBuilder<> *IRB, llvm::Value *argL, llvm::Value
// *argR) {
//   auto cmpRes = new llvm::ICmpInst(*b, llvm::CmpInst::ICMP_ULT, argL, argR);
//   F_WRITE(b, llvm::X86::CF, cmpRes);
// }
//
// template <int width>
// static void WriteOFSub(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *lhs,
//                        llvm::Value *rhs) {
//   // of = lshift((lhs ^ rhs ) & (lhs ^ res), 12 - width) & 2048
//   // where lshift is written as if n >= 0, x << n, else x >> (-n)
//   auto xor1 = IRB->CreateXor(lhs, rhs, "", b);
//   auto xor2 = IRB->CreateXor(lhs, res, "", b);
//   auto anded = IRB->CreateAnd(xor1, xor2, "", b);
//
//   llvm::Value *shifted = nullptr;
//   // extract sign bit
//   switch (width) {
//     case 8:
//     case 16:
//     case 32:
//     case 64:
//       shifted = IRB->CreateLShr(
//           anded, CONST_V<width>(b, width - 1), "", b);
//       break;
//
//     default:
//       throw TErr(__LINE__, __FILE__, "Invalid bitwidth");
//   }
//
//   // truncate anded1
//   auto &C = b->getContext();
//   auto trunced = new llvm::ZExtInst(
//       new llvm::TruncInst(shifted, llvm::Type::getInt1Ty(C), "", b),
//       llvm::Type::getInt8Ty(C), "", b);
//
//   // write to OF
//   F_WRITE(b, llvm::X86::OF, trunced);
// }
//
// template <int width>
// static void WriteOFAdd(IRBuilder<> *IRB, llvm::Value *res, llvm::Value *lhs,
//                        llvm::Value *rhs) {
//   // of = lshift((lhs ^ rhs ^ -1) & (lhs ^ res), 12 - width) & 2048
//   // where lshift is written as if n >= 0, x << n, else x >> (-n)
//
//   auto xor1 = IRB->CreateXor(lhs, rhs, "", b);
//   auto xor2 =
//       IRB->CreateXor(xor1, CONST_V<width>(b, -1), "", b);
//   auto xor3 = IRB->CreateXor(lhs, res, "", b);
//   auto anded = IRB->CreateAnd(xor2, xor3, "", b);
//   llvm::Value *shifted = nullptr;
//   // shifts corrected to always place the OF bit
//   // in the bit 0 position. This way it works for
//   // all sized ints
//   switch (width) {
//     case 8:
//       // lshift by 4
//       shifted = IRB->CreateLShr(
//           anded, CONST_V<width>(b, 11 - 4), "", b);
//       break;
//     // in these two cases, we rshift instead
//     case 16:
//       // rshift by 4
//       shifted = IRB->CreateLShr(
//           anded, CONST_V<width>(b, 11 + 4), "", b);
//       break;
//     case 32:
//       // rshift by 20
//       shifted = IRB->CreateLShr(
//           anded, CONST_V<width>(b, 11 + 20), "", b);
//       break;
//     case 64:
//       // rshift by 52
//       shifted = IRB->CreateLShr(
//           anded, CONST_V<width>(b, 11 + 52), "", b);
//       break;
//
//     default:
//       throw TErr(__LINE__, __FILE__, "Invalid bitwidth");
//   }
//
//   // and by 1
//   auto anded1 =
//       IRB->CreateAnd(shifted, CONST_V<width>(b, 1), "", b);
//
//   // truncate anded1
//   auto &C = b->getContext();
//   auto trunced = new llvm::ZExtInst(
//       new llvm::TruncInst(anded1, llvm::Type::getInt1Ty(C), "", b),
//       llvm::Type::getInt8Ty(C), "", b);
//
//   // write to OF
//   F_WRITE(b, llvm::X86::OF, trunced);
// }
//
// template <int width>
// static void WritePF(IRBuilder<> *IRB, llvm::Value *written) {
//   auto M = b->getParent()->getParent();
//   // truncate the written value to one byte
//   // if the width is already 8, we don't need to do this
//   llvm::Value *lsb = nullptr;
//   if (width > 8) {
//     lsb = new llvm::TruncInst(written,
//     llvm::Type::getInt8Ty(b->getContext()),
//                               "", b);
//   } else {
//     lsb = written;
//   }
//
//   // use llvm.ctpop.i8 to count the bits set in this byte
//   auto &C = b->getContext();
//   llvm::Type *s[] = {llvm::Type::getInt8Ty(C)};
//   auto popCntFun =
//       llvm::Intrinsic::getDeclaration(M, llvm::Intrinsic::ctpop, s);
//
//   std::vector<llvm::Value *> countArgs;
//   countArgs.push_back(lsb);
//
//   auto count = llvm::CallInst::Create(popCntFun, countArgs, "", b);
//
//   // truncate the count to a bit
//   auto ty = llvm::Type::getInt1Ty(C);
//   auto countTrunc = new llvm::ZExtInst(new llvm::TruncInst(count, ty, "", b),
//                                        llvm::Type::getInt8Ty(C), "", b);
//
//   // negate that bit via xor 1
//   auto neg =
//       IRB->CreateXor(countTrunc, CONST_V<8>(b, 1), "", b);
//
//   // write that bit to PF
//   F_WRITE(b, llvm::X86::PF, neg);
// }
//
// template <int width>
// static void WriteSF(IRBuilder<> *IRB, llvm::Value *written) {
//   //%1 = SIGNED CMP %written < 0
//   // Value   *scmp = new ICmpInst(   *b,
//   //                                ICmpInst::ICMP_SLT,
//   //                                written,
//   //                                CONST_V<width>(b, 0));
//
//   // extract sign bit
//   auto signBit = IRB->CreateLShr(
//       written, CONST_V<width>(b, width - 1), "", b);
//   auto &C = b->getContext();
//   auto trunc = new llvm::ZExtInst(
//       new llvm::TruncInst(signBit, llvm::Type::getInt1Ty(C), "", b),
//       llvm::Type::getInt8Ty(C), "", b);
//   F_WRITE(b, llvm::X86::SF, trunc);
// }
