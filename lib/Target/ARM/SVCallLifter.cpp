#include "Target/ARM/SVCallLifter.h"

#include "ARMBaseInfo.h"
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
  Value* reg = NULL;
  Value* sp = ReadReg(Reg("SP"), IRB, 32);

  llvm::StringRef targets[] = {"xPSR", "PC", "LR", "R12",
                               "R3",   "R2", "R1", "R0"};

  uint32_t PC = alm->Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());

  std::stringstream stream;
  stream << std::hex << (PC-2);
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

  std::vector<Type*> FuncTy_3_args;
  FunctionType* FuncTy_3 = FunctionType::get(
      /*Result=*/Type::getVoidTy(alm->Mod->getContext()),
      /*Params=*/FuncTy_3_args,
      /*isVarArg=*/false);

  PointerType* PointerTy_1 = PointerType::get(FuncTy_3, 0);

  std::vector<Type*> FuncTy_4_args;
  FunctionType* FuncTy_4 = FunctionType::get(
      /*Result=*/Type::getVoidTy(alm->Mod->getContext()),
      /*Params=*/FuncTy_4_args,
      /*isVarArg=*/true);

  // Function Declarations
  Function* func_inception_dump_registers =
      alm->Mod->getFunction("inception_sv_call");
  if (!func_inception_dump_registers) {
    func_inception_dump_registers = Function::Create(
        /*Type=*/FuncTy_4,
        /*Linkage=*/GlobalValue::ExternalLinkage,
        /*Name=*/"inception_sv_call", alm->Mod);  // (external, no body)
    func_inception_dump_registers->setCallingConv(CallingConv::C);
  }

  // AttributeSet func_inception_dump_registers_PAL;
  // {
  //   SmallVector<AttributeSet, 4> Attrs;
  //   AttributeSet PAS;
  //   {
  //     AttrBuilder B;
  //     PAS = AttributeSet::get(alm->Mod->getContext(), ~0U, B);
  //   }
  //
  //   Attrs.push_back(PAS);
  //   func_inception_dump_registers_PAL =
  //       AttributeSet::get(alm->Mod->getContext(), Attrs);
  // }
  // func_inception_dump_registers->setAttributes(
  //     func_inception_dump_registers_PAL);

  // Constant Definitions
  Constant* const_ptr_7 = ConstantExpr::getCast(
      Instruction::BitCast, func_inception_dump_registers, PointerTy_1);
  std::vector<Value*> params;

  Instruction* call = IRB->CreateCall(const_ptr_7, params);

  // GlobalVariable* gvar = dyn_cast<GlobalVariable>(reg);

  // MDNode* Node =
  //     MDNode::get(alm->Mod->getContext(),
  //                 MDString::get(alm->Mod->getContext(), std::to_string(PC)));
  //
  // call->setMetadata("FixedAllocation", Node);
  // gvar->addAttribute(StringRef("FixedAllocation"),
  //                    StringRef(std::to_string(PC)));
}
