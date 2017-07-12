#include "Target/ARM/LoadLifter.h"

#include "ARMBaseInfo.h"
#include "CodeInv/Decompiler.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

using namespace llvm;
using namespace fracture;

SDNode *LoadLifter::select(SDNode *N) {
  uint16_t TargetOpc = N->getMachineOpcode();

  return NULL;
}

void LoadLifter::InvLoadMultiple(SDNode *N, bool Inc, bool Before,
                                 LoadInfo *info) {
  SDValue Chain = N->getOperand(info->chain);
  SDValue Ptr = N->getOperand(info->addr);

  uint16_t MathOpc = (Inc) ? ISD::ADD : ISD::SUB;

  SDValue PrevPtr = Ptr;
  SDValue UsePtr;

  SDValue Offset = ARMLifterManager::DAG->getConstant(4, EVT(MVT::i32), false);

  SDLoc SL(N);

  SDVTList VTList = ARMLifterManager::DAG->getVTList(MVT::i32);

  EVT LdType = N->getValueType(0);

  unsigned ImmSum = 0;

  for (unsigned i = info->src_begin; i < info->src_end; ++i) {
    SDValue Val = N->getOperand(i);

    Ptr = ARMLifterManager::DAG->getNode(MathOpc, SL, VTList, Ptr, Offset);

    ImmSum += 4;

    Value *NullPtr = 0;

    MachineMemOperand *MMO = new MachineMemOperand(
        MachinePointerInfo(NullPtr, ImmSum), MachineMemOperand::MOStore, 4, 0);

    // Before or after behavior
    UsePtr = (Before) ? PrevPtr : Ptr;

    SDValue ResNode;

    // This is a special case, because we have to flip CopyFrom/To
    if (Val->hasNUsesOfValue(1, 0)) {
      Chain = Val->getOperand(0);
    }

    // Check if we are loading into PC, if we are, emit a return.
    RegisterSDNode *RegNode = dyn_cast<RegisterSDNode>(Val->getOperand(1));

    const MCRegisterInfo *RI = ARMLifterManager::Dec->getDisassembler()
                                   ->getMCDirector()
                                   ->getMCRegisterInfo();
    if (RI->isSubRegisterEq(RI->getProgramCounter(), RegNode->getReg())) {
      ResNode = ARMLifterManager::DAG->getNode(
          ARMISD::RET_FLAG, SL, MVT::Other, ARMLifterManager::DAG->getRoot());
      ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Chain);
      ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Ptr);
      ARMLifterManager::DAG->setRoot(ResNode);
      return;
    } else {
      ResNode = ARMLifterManager::DAG->getLoad(
          LdType, SL, Chain, UsePtr, MachinePointerInfo::getConstantPool(),
          false, false, true, 0);
      SDValue C2R = ARMLifterManager::DAG->getCopyToReg(
          ResNode.getValue(1), SL, RegNode->getReg(), ResNode);
      Chain = C2R;
      // If CopyFromReg has only 1 use, replace it
      if (Val->hasNUsesOfValue(1, 0)) {
        ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(Val, ResNode);
        ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(Val.getValue(1),
                                                         ResNode.getValue(1));
        ARMLifterManager::DAG->DeleteNode(Val.getNode());
      }
    }

    if (ResNode.getNumOperands() > 1) {
      ARMLifterManager::FixChainOp(ResNode.getNode());
    }
    PrevPtr = Ptr;
  }

  // NOTE: This gets handled automagically because DAG represents it, but leave
  // for now in case we need it.
  // Writeback operation
  // if (WB) {
  //   RegisterSDNode *WBReg =
  //     dyn_cast<RegisterSDNode>(N->getOperand(1)->getOperand(1));
  //   Chain = ARMLifterManager::DAG->getCopyToReg(Ptr, SL, WBReg->getReg(),
  //   Chain);
  // }

  // Pass the chain to the last chain node
  ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Chain);

  // Pass the last math operation to any uses of Rn
  ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Ptr);
}

// if( is_pc_affected(Val) )
//   create_ret(info->N, info->Chain, info->Ptr);
// else

// bool StoreLifter::is_pc_affected(SDValue Val) {
//   // Check if we are loading into PC, if we are, emit a return.
//   if(Val->getNumOperands()>=2) {
//
//     RegisterSDNode *RegNode = dyn_cast<RegisterSDNode>(Val->getOperand(1));
//
//     const MCRegisterInfo *RI = ARMLifterManager::Dec->getDisassembler()
//     ->getMCDirector()
//     ->getMCRegisterInfo();
//
//     return (RI->isSubRegisterEq(RI->getProgramCounter(), RegNode->getReg()));
//   }
//   return false;
// }
//
// SDValue StoreLifter::create_ret(SDNode *N, SDValue Chain, SDValue Ptr) {
//   SDLoc SL(N);
//
//   llvm::errs() << "PC affected -> return\n";
//
//   SDValue ResNode = ARMLifterManager::DAG->getNode(
//       ARMISD::RET_FLAG, SL, MVT::Other, ARMLifterManager::DAG->getRoot());
//   ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, 1),
//                                                    Chain);
//   ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, 0),
//                                                    Ptr);
//   ARMLifterManager::DAG->setRoot(ResNode);
//
//   return ResNode;
// }
