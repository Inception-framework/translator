#include "Target/ARM/LoadLifter.h"

#include "ARMBaseInfo.h"
#include "CodeInv/Decompiler.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;
using namespace fracture;

SDNode *LoadLifter::select(SDNode *N) {
  uint16_t TargetOpc = N->getMachineOpcode();

  switch (TargetOpc) {
    case ARM::tPOP:
      // SDNode *_N, _chain, _src_begin, _src_end, _addr, _offset, _inc, _before
      InvLoadMultiple(new LoadInfo(N, 0, 3, N->getNumOperands() - 1,
                                   N->getNumOperands() - 1, -1, true, false));
      break;
    case ARM::t2LDRi12:
      // SDNode *_N, _chain, _src_begin, _src_end, _addr, _offset, _inc, _before
      InvLoadMultiple(new LoadInfo(N, 0, 4, 4, 1, 2, true, false));
      break;
    case ARM::t2LDMIA_UPD:
      // SDNode *_N, _chain, _src_begin, _src_end, _addr, _offset, _inc, _before
      InvLoadMultiple(
          new LoadInfo(N, 0, 4, 4, N->getNumOperands() - 1, -1, true, false));
      break;
  }

  return NULL;
}

void LoadLifter::InvLoadMultiple(LoadInfo *info) {
  // Initial Value
  info->Chain = info->N->getOperand(info->chain);
  info->Ptr = info->N->getOperand(info->addr);

  info->MathOpc = (info->Increment) ? ISD::ADD : ISD::SUB;

  // Is there any offset
  if (info->offset == -1) {
    // The DAG Node does not contain any offset
    // Offset is not available in multi load
    // We create our own offset
    info->Offset = ARMLifterManager::DAG->getConstant(4, EVT(MVT::i32), false);

  } else {
    info->Offset = info->N->getOperand(info->offset);
  }

  // For each source value to load
  for (unsigned i = info->src_begin; i < info->src_end; ++i) {
    SDValue Val = info->N->getOperand(i);

    Val->dump();

    // if (Val->hasNUsesOfValue(1, 0)) {
    //   info->Chain = Val->getOperand(0);
    // }

    // Update before ?
    if (info->Before)
      info->Ptr =
          update_pointer(info->N, info->MathOpc, info->Ptr, info->Offset,
                         info->offset == -1 && info->Before ? true : false);

    // if (is_pc_affected(Val))
    //   create_ret(info->N, info->Chain, info->Ptr);
    // else
    info->Chain = create_load(info->N, Val, info->Chain, info->Ptr);

    // Update after ?
    if (!info->Before)
      info->Ptr =
          update_pointer(info->N, info->MathOpc, info->Ptr, info->Offset,
                         info->offset == -1 && info->Before ? true : false);

    update(info->Chain);
  }

  clean_graph(info->N, info->Chain, info->Ptr);
}

void LoadLifter::update(SDValue ResNode) {}

SDValue LoadLifter::update_pointer(SDNode *N, uint16_t MathOpc, SDValue Ptr,
                                   SDValue Offset, bool UpdateRef) {
  SDLoc SL(N);

  SDVTList VTList = ARMLifterManager::DAG->getVTList(MVT::i32);

  // Compute new pointer value
  SDValue ptr =
      ARMLifterManager::DAG->getNode(MathOpc, SL, VTList, Ptr, Offset);

  // if (UpdateRef) {
  //   ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(
  //       SDValue(Offset.getNode(), 0), ptr);
  // }

  return ptr;
}

SDValue LoadLifter::create_load(SDNode *N, SDValue Val, SDValue Chain,
                                SDValue Ptr) {
  llvm::errs() << "Create load\n";

  SDLoc SL(N);

  EVT LdType = N->getValueType(0);

  RegisterSDNode *RegNode = dyn_cast<RegisterSDNode>(Val->getOperand(1));

  SDValue ResNode = ARMLifterManager::DAG->getLoad(
      LdType, SL, Chain, Ptr, MachinePointerInfo::getConstantPool(), false,
      false, true, 0);
  SDValue C2R = ARMLifterManager::DAG->getCopyToReg(ResNode.getValue(1), SL,
                                                    RegNode->getReg(), ResNode);

  if (ResNode.getNumOperands() > 1) {
    ARMLifterManager::FixChainOp(ResNode.getNode());
  }
  // if (Val->hasNUsesOfValue(1, 0)) {
  //   ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(Val, ResNode);
  //   ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(Val.getValue(1),
  //                                     ResNode.getValue(1));
  //   ARMLifterManager::DAG->DeleteNode(Val.getNode());
  // }

  return C2R;
}

void LoadLifter::clean_graph(SDNode *N, SDValue Chain, SDValue Ptr) {
  if (N->getNumValues() >= 2) {
    // Pass the last math operation to any uses of Rn
    ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Ptr);
    // Pass the chain to the last chain node
    ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Chain);
  } else if (N->getNumValues() == 1) {
    // Pass the chain to the last chain node
    ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Chain);
  }
}

bool LoadLifter::is_pc_affected(SDValue Val) {
  // Check if we are loading into PC, if we are, emit a return.
  if (Val->getNumOperands() >= 2) {
    RegisterSDNode *RegNode = dyn_cast<RegisterSDNode>(Val->getOperand(1));

    const MCRegisterInfo *RI = ARMLifterManager::Dec->getDisassembler()
                                   ->getMCDirector()
                                   ->getMCRegisterInfo();

    return (RI->isSubRegisterEq(RI->getProgramCounter(), RegNode->getReg()));
  }
  return false;
}

SDValue LoadLifter::create_ret(SDNode *N, SDValue Chain, SDValue Ptr) {
  SDLoc SL(N);

  SDValue ResNode = ARMLifterManager::DAG->getNode(
      ARMISD::RET_FLAG, SL, MVT::Other, ARMLifterManager::DAG->getRoot());
  ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Chain);
  ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Ptr);
  ARMLifterManager::DAG->setRoot(ResNode);

  return ResNode;
}
