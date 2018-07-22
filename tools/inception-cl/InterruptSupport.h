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


#ifndef INTERRUPT_SUPPORT_H
#define INTERRUPT_SUPPORT_H

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "llvm/ADT/StringRef.h"

#include "Transforms/NameRecovery.h"
#include "llvm/PassManager.h"

#include "Utils/Builder.h"
#include "Utils/IContext.h"

class InterruptSupport {
 public:
  static void WriteInterruptPrologue(llvm::StringRef handler) {
    if (handler.find("SVC") != StringRef::npos) return;

    Function* function = IContext::Mod->getFunction(handler);

    if (function == NULL) {
      inception_warning("Cannot write IRQ prologue for unknonwn function %s.",
                        handler.str().c_str());
      return;
    }

    if (function->empty()) {
      inception_message("Writing interrupt prologue to %s failed",
                        handler.str().c_str());
      return;
    }

    inception_message("Writing interrupt prologue to %s",
                      handler.str().c_str());

    llvm::IRBuilder<>* IRB = new IRBuilder<>(function->getEntryBlock().begin());

    Value* reg = NULL;
    Value* sp = ReadReg(Reg("SP"), IRB, 32);

    llvm::StringRef targets[] = {"xPSR", "PC", "LR", "R12",
                                 "R3",   "R2", "R1", "R0"};

    Type* Ty_word = IntegerType::get(IContext::getContextRef(), 32);

    AllocaInst* APSR = IRB->CreateAlloca(Ty_word);

    for (auto target : targets) {
      /*xPSR is build at run-time*/
      if (target.equals("xPSR")) {
        // APSR |= ( NF & 0x1 ) << 31;
        // APSR |= ( ZF & 0x1 ) << 30;
        // APSR |= ( CF & 0x1 ) << 29;
        // APSR |= ( VF & 0x1 ) << 28;
        // APSR |= ( QF & 0x1 ) << 27;

        Value* flags[5];
        flags[0] = Reg("NF");
        flags[1] = Reg("ZF");
        flags[2] = Reg("CF");
        flags[3] = Reg("VF");
        flags[4] = Reg("QF");

        IRB->CreateStore(getConstant(0), APSR);

        uint32_t shift = 31;
        for (auto flag : flags) {
          Value* val = IRB->CreateLoad(flag);
          val = IRB->CreateAnd(val, getConstant(1));
          val = IRB->CreateShl(val, getConstant(shift--));
          Value* apsr = IRB->CreateLoad(APSR);
          val = IRB->CreateOr(apsr, val);
          IRB->CreateStore(val, APSR);
        }
        reg = IRB->CreateLoad(APSR);

      } else {
        reg = ReadReg(Reg(target), IRB, 32);
      }
      sp = UpdateRd(sp, getConstant("4"), IRB, false);
      WriteReg(reg, sp, IRB, 32);
    }

    WriteReg(sp, Reg("SP"), IRB, 32);
    WriteReg(getConstant("4"), Reg("LR"), IRB, 32);

    inception_message("Done");
  }

  static void WriteInterruptEpilogue(llvm::StringRef handler) {
    Function* function = IContext::Mod->getFunction(handler);

    if (function == NULL) {
      inception_warning("Cannot write IRQ epilogue for unknonwn function %s.",
                        handler.str().c_str());
      return;
    }

    if (function->empty()) {
      inception_message("Writing interrupt epilogue to %s failed",
                        handler.str().c_str());
      return;
    }

    inception_message("Writing interrupt epilogue to %s",
                      handler.str().c_str());

    Instruction* last = GetLastInstruction(function);
    if (last == NULL) {
      inception_error("Handler %s has no return instruction",
                      handler.str().c_str());
    }

    llvm::IRBuilder<>* IRB = new IRBuilder<>(last);

    Value* reg = NULL;
    Value* sp = ReadReg(Reg("SP"), IRB, 32);

    llvm::StringRef targets[] = {"R0",  "R1", "R2", "R3",
                                 "R12", "LR", "PC", "xPSR"};

    // Type* Ty_word = IntegerType::get(IContext::getContextRef(), 32);

    // AllocaInst* APSR = IRB->CreateAlloca(Ty_word);

    for (auto target : targets) {
      /*xPSR is build at run-time*/
      if (target.equals("xPSR")) {
        // APSR |= ( NF & 0x1 ) << 31;
        // APSR |= ( ZF & 0x1 ) << 30;
        // APSR |= ( CF & 0x1 ) << 29;
        // APSR |= ( VF & 0x1 ) << 28;
        // APSR |= ( QF & 0x1 ) << 27;

        Value* flags[5];
        flags[0] = Reg("NF");
        flags[1] = Reg("ZF");
        flags[2] = Reg("CF");
        flags[3] = Reg("VF");
        flags[4] = Reg("QF");

        // Retrieve APSR from stack
        reg = ReadReg(sp, IRB, 32);

        // Dispatch APSR into flags
        uint32_t shift = 31;
        for (auto flag : flags) {
          // 0x1 << shift;
          Value* val = IRB->CreateShl(getConstant(1), getConstant(shift));
          // AND APSR -> retrieve bit shift
          val = IRB->CreateAnd(reg, val);
          // Shift to the other direction
          val = IRB->CreateLShr(val, getConstant(shift--));

          WriteReg(val, flag, IRB, 32);
        }
        sp = UpdateRd(sp, getConstant("4"), IRB, true);
      } else {
        reg = ReadReg(sp, IRB, 32);
        WriteReg(reg, Reg(target), IRB, 32);
        sp = UpdateRd(sp, getConstant("4"), IRB, true);
      }
    }

    // FunctionPassManager FPM(IContext::Mod);
    // FPM.add(createNameRecoveryPass());
    // FPM.run(*function);

    inception_message("Done");
  }

 private:
  static Instruction* GetLastInstruction(llvm::Function* fct) {
    auto b = fct->begin();
    while (b != fct->end()) {
      auto i = b->begin();
      while (i != b->end()) {
        ReturnInst* ret = dyn_cast<ReturnInst>(i);
        if (ret != NULL) return (Instruction*)i--;
        i++;
      }
      b++;
    }

    return NULL;
  }
};
#endif
