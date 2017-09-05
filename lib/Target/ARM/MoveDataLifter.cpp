#include "Target/ARM/MoveDataLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"
#include "Target/ARM/ShiftLifter.h"

using namespace llvm;
using namespace fracture;

void MoveDataLifter::registerLifter() {
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MOVi16,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MOVi,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::tMOVSr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::tMOVi8,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::tMOVr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MOVr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MOVr,
                      (LifterHandler)&MoveDataLifter::MoveHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MOVi,
                      (LifterHandler)&MoveDataLifter::MoveHandler);

  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNCCi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNr,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNsi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::MVNsr,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MVNr,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MVNi,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"), (unsigned)ARM::tMVN,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
  alm->registerLifter(this, std::string("MoveDataLifter"),
                      (unsigned)ARM::t2MVNs,
                      (LifterHandler)&MoveDataLifter::MoveNotHandler);
}

void MoveDataLifter::MoveHandler(llvm::SDNode* N, IRBuilder<>* IRB) {

  Value* Op0 = visit(N->getOperand(0).getNode(), IRB);

  if (IsSetFlags(N)) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter* flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    ////Compute NF
    // flags->WriteNFAdd(IRB, Res);
    // Compute NF.
    flags->WriteNF(IRB, Op0);
    // Compute ZF.
    flags->WriteZF(IRB, Op0);
    ////TODO Compute CF. in case of shift only??
    // flags->WriteCFShiftR(IRB, partial_res);
    const ConstantSDNode* ConstNode =
        dyn_cast<ConstantSDNode>(N->getOperand(0));
    if (ConstNode) {
      uint32_t constant = ConstNode->getZExtValue();
      flags->WriteCFconstant(IRB, constant);
    }
  }

  saveNodeValue(N, Op0);
}

void MoveDataLifter::MoveNotHandler(llvm::SDNode* N, IRBuilder<>* IRB) {

  llvm::outs() << "MoveNotHandler\n";
  Value* Op0 = NULL;
  unsigned opcode = N->getMachineOpcode();
  if (opcode == ARM::t2MVNs) {
    ShiftLifter* shiftLifter = dyn_cast<ShiftLifter>(alm->resolve("SHIFT"));
    shiftLifter->ShiftHandlerShiftOp(N, IRB);
    Op0 = getSavedValue(N);
  } else {
    Op0 = visit(N->getOperand(0).getNode(), IRB);
  }

  Value* Res = IRB->CreateNot(Op0);

  if (IsSetFlags(N)) {
    // Write the flag updates.
    // Compute AF.
    FlagsLifter* flags = dyn_cast<FlagsLifter>(alm->resolve("FLAGS"));

    ////Compute NF
    // flags->WriteNFAdd(IRB, Res);
    // Compute NF.
    flags->WriteNF(IRB, Res);
    // Compute ZF.
    flags->WriteZF(IRB, Res);
    ////TODO Compute CF. in case of shift only??
    if (opcode != ARM::t2MVNs) {
      const ConstantSDNode* ConstNode =
          dyn_cast<ConstantSDNode>(N->getOperand(0));
      if (ConstNode) {
        uint32_t constant = ConstNode->getZExtValue();
        flags->WriteCFconstant(IRB, constant);
      } else {
        flags->WriteCFShiftL(IRB, Op0, getConstant("0"));
      }
    }
  }

  saveNodeValue(N, Res);
}
