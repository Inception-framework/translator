
#ifndef INTERRUPT_SUPPORT_H
#define INTERRUPT_SUPPORT_H

#include "Utils/Builder.h"
#include "Utils/IContext.h"

void WriteInterruptPrologue() {
  Value* reg = NULL;
  Value* sp = ReadReg(Reg("SP"), IRB, 32);

  llvm::StringRef targets[] = {"xPSR", "PC", "LR", "R12",
                               "R3",   "R2", "R1", "R0"};

  uint32_t PC = alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());

  std::stringstream stream;
  stream << std::hex << (PC - 2);
  std::string PC_hex(stream.str());

  StringRef reg_name("_SVC_" + PC_hex + "\0");
  reg = Reg(reg_name);
  WriteReg(visit(N->getOperand(1).getNode(), IRB), reg, IRB, 32);

  WriteReg(getConstant(PC), Reg("PC"), IRB, 32);

  for (auto target : targets) {
    /*xPSR is build at run-time*/
    if (target.equals("xPSR")) {
      // APSR |= ( NF & 0x1 ) << 31;
      // APSR |= ( ZF & 0x1 ) << 30;
      // APSR |= ( CF & 0x1 ) << 29;
      // APSR |= ( VF & 0x1 ) << 28;
      // APSR |= ( QF & 0x1 ) << 27;

      Value flags[] = {Reg("NF"), Reg("ZF"), Reg("CF"), Reg("VF"), Reg("QF")};

      Type* Ty_word = IntegerType::get(mod->getContext(), 32);

      AllocaInst* APSR = IRB->CreateAlloca(Ty_word, "apsr");
      IRB->CreateStore(getConstant(0), APSR);

      uint32_t shift = 31;
      for (auto flag : flags) {
        Value* val = IRB->CreateLoad(flag);
        val = IRB->CreateAND(val, getConstant(1));
        val = IRB->CreateShl(val, getConstant(shift--));
        Value* apsr = IRB->CreateLoad(APSR);
        val = IRB->CreateOr(apsr, val);
        IRB->CreateStore(val, APSR);
      }

      return APSR;
    }

    reg = ReadReg(Reg(target), IRB, 32);
    sp = UpdateRd(sp, getConstant("4"), IRB, false);
    WriteReg(reg, sp, IRB, 32);
  }

  WriteReg(sp, Reg("SP"), IRB, 32);
  WriteReg(getConstant("4"), Reg("LR"), IRB, 32);
}

void WriteInterruptEpilogue() {
  // Value* reg = NULL;
  // Value* sp = ReadReg(Reg("SP"), IRB, 32);
  //
  // llvm::StringRef targets[] = {"xPSR", "PC", "LR", "R12",
  //                              "R3",   "R2", "R1", "R0"};
  //
  // uint32_t PC =
  // alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  //
  // std::stringstream stream;
  // stream << std::hex << (PC - 2);
  // std::string PC_hex(stream.str());
  //
  // StringRef reg_name("_SVC_" + PC_hex + "\0");
  // reg = Reg(reg_name);
  // WriteReg(visit(N->getOperand(1).getNode(), IRB), reg, IRB, 32);
  //
  // WriteReg(getConstant(PC), Reg("PC"), IRB, 32);
  //
  // for (auto target : targets) {
  //   reg = ReadReg(Reg(target), IRB, 32);
  //   sp = UpdateRd(sp, getConstant("4"), IRB, false);
  //   WriteReg(reg, sp, IRB, 32);
  // }
  //
  // WriteReg(sp, Reg("SP"), IRB, 32);
  // WriteReg(getConstant("4"), Reg("LR"), IRB, 32);
}
#endif
