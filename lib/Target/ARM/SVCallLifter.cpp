#include "Target/ARM/SVCallLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"

#include "Utils/IContext.h"

using namespace llvm;
using namespace fracture;

void SVCallLifter::registerLifter() {
  alm->registerLifter(this, std::string("SVCALL"), (unsigned)ARM::tSVC,
                      (LifterHandler)&SVCallLifter::SVCallHandler);
}

/*
 * if priviledged
 *   SP = MSP
 *   LR = 0x0
 * else
 *   SP = PSP
 *   LR = 0x4
 *
 * PUSH {R0-R3, R12, LR, PC, xPSR}
 * PC = &(SVCHandler)
 *
 */
void SVCallLifter::SVCallHandler(SDNode* N, IRBuilder<>* IRB) {
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
    reg = ReadReg(Reg(target), IRB, 32);
    sp = UpdateRd(sp, getConstant("4"), IRB, false);
    WriteReg(reg, sp, IRB, 32);
  }

  WriteReg(sp, Reg("SP"), IRB, 32);
  WriteReg(getConstant("4"), Reg("LR"), IRB, 32);

  Constant* fct_ptr = GetVoidFunctionPointer("inception_sv_call");
  std::vector<Value*> params;

  IRB->CreateCall(fct_ptr, params);
}
