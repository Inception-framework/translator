
#ifndef INTERRUPT_SUPPORT_H
#define INTERRUPT_SUPPORT_H

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "Utils/Builder.h"
#include "Utils/IContext.h"

class InterruptSupport {
 public:
  static void WriteInterruptPrologue(llvm::StringRef handler) {
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

    AllocaInst* APSR = IRB->CreateAlloca(Ty_word, getConstant(0));

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
  }
};
#endif
