#include "Target/ARM/SVCallLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"

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

  std::vector<Type*> FuncTy_3_args;
  FunctionType* FuncTy_3 = FunctionType::get(
      /*Result=*/Type::getVoidTy(getModule()->getContext()),
      /*Params=*/FuncTy_3_args,
      /*isVarArg=*/false);

  PointerType* PointerTy_1 = PointerType::get(FuncTy_3, 0);

  std::vector<Type*> FuncTy_4_args;
  FunctionType* FuncTy_4 = FunctionType::get(
      /*Result=*/Type::getVoidTy(getModule()->getContext()),
      /*Params=*/FuncTy_4_args,
      /*isVarArg=*/true);

  // Function Declarations
  Function* func_inception_dump_registers =
      getModule()->getFunction("inception_sv_call");
  if (!func_inception_dump_registers) {
    func_inception_dump_registers = Function::Create(
        /*Type=*/FuncTy_4,
        /*Linkage=*/GlobalValue::ExternalLinkage,
        /*Name=*/"inception_sv_call", getModule());  // (external, no body)
    func_inception_dump_registers->setCallingConv(CallingConv::C);
  }

  // Constant Definitions
  Constant* const_ptr_7 = ConstantExpr::getCast(
      Instruction::BitCast, func_inception_dump_registers, PointerTy_1);
  std::vector<Value*> params;

  IRB->CreateCall(const_ptr_7, params);
}
