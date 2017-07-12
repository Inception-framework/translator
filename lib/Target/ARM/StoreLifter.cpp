#include "Target/ARM/StoreLifter.h"

#include "ARMBaseInfo.h"
#include "CodeInv/Decompiler.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;
using namespace fracture;

SDNode *StoreLifter::select(SDNode *N) {
  uint16_t TargetOpc = N->getMachineOpcode();

  switch (TargetOpc) {
    case ARM::t2STRBT:
    case ARM::t2STRB_PRE:
    case ARM::t2STRB_preidx:
    case ARM::t2STRBi12:
    case ARM::t2STRBi8:
    case ARM::t2STRBs:
    case ARM::t2STRD_PRE:
    case ARM::t2STRDi8:
    case ARM::t2STREX:
    case ARM::t2STREXB:
    case ARM::t2STREXD:
    case ARM::t2STREXH:
    case ARM::t2STRHT:
    case ARM::t2STRH_PRE:
    case ARM::t2STRH_preidx:
    case ARM::t2STRHi8:
    case ARM::t2STRHs:
    case ARM::t2STRT:
    case ARM::t2STR_PRE:
    case ARM::t2STR_preidx:
    case ARM::t2STRi8:
    case ARM::t2STRs:
    case ARM::tSTRr:
    // case ARM::t2STARM::RDi8:
    case ARM::t2STRHi12:
    case ARM::t2STRi12:
      InvStoreMultiple(new StoreInfo(N, 0, 1, 2, 2, 3, true, true));
      break;
    case ARM::t2STRB_POST:
    case ARM::t2STRH_POST:
    case ARM::t2STRD_POST:
    case ARM::t2STR_POST:
      InvStoreMultiple(new StoreInfo(N, 0, 1, 2, 2, 3, true, false));
      break;
    case ARM::t2STMIA_UPD:
      // SDNode *_N, _chain, _src_begin, _src_end, _addr, _offset, _inc, _before
      InvStoreMultiple(
          new StoreInfo(N, 0, 4, N->getNumOperands(), 1, -1, true, false));
      break;
    case ARM::tPUSH:
      // SDNode *_N, _chain, _src_begin, _src_end, _addr, _offset, _inc, _before
      InvStoreMultiple(new StoreInfo(N, 0, 3, N->getNumOperands() - 1,
                                     N->getNumOperands() - 1, -1, true, false));
      break;
    case ARM::t2STMDB:
    case ARM::t2STMDB_UPD:
      // SDNode *_N, _chain, _src_begin, _src_end, _addr, _offset, _inc, _before
      InvStoreMultiple(
          new StoreInfo(N, 0, 4, N->getNumOperands(), 1, -1, true, false));
      break;
      // case ARM::t2STMIA_UPD:
      // case ARM::t2STR_PRE:
      // case ARM::t2STR_POST:
  }

  return NULL;
}

void StoreLifter::InvStoreMultiple(StoreInfo *info) {
  // Initial Value
  info->Chain = info->N->getOperand(info->chain);
  info->Ptr = info->N->getOperand(info->addr);

  info->MathOpc = (info->Increment) ? ISD::ADD : ISD::SUB;

  // Is there any offset
  if (info->offset == -1) {
    // The DAG Node does not contain any offset
    // Offset is not available in multi store
    // We create our own offset
    info->Offset = ARMLifterManager::DAG->getConstant(4, EVT(MVT::i32), false);

  } else {
    info->Offset = info->N->getOperand(info->offset);
  }

  // For each source value to store
  for (unsigned i = info->src_begin; i < info->src_end; ++i) {
    SDValue Val = info->N->getOperand(i);

    // Update before ?
    if (info->Before)
      info->Ptr =
          update_pointer(info->N, info->MathOpc, info->Ptr, info->Offset,
                         info->offset == -1 && info->Before ? true : false);

    info->Chain =
        create_store(info->N, Val, info->Chain, info->Ptr, &info->ImmSum);

    // Update after ?
    if (!info->Before)
      info->Ptr =
          update_pointer(info->N, info->MathOpc, info->Ptr, info->Offset,
                         info->offset == -1 && info->Before ? true : false);

    update(info->Chain);
  }

  clean_graph(info->N, info->Chain, info->Ptr);
}

void StoreLifter::update(SDValue ResNode) {
  if (ResNode.getNumOperands() > 1) {
    ARMLifterManager::FixChainOp(ResNode.getNode());
  }
}

SDValue StoreLifter::update_pointer(SDNode *N, uint16_t MathOpc, SDValue Ptr,
                                    SDValue Offset, bool UpdateRef) {
  SDLoc SL(N);

  SDVTList VTList = ARMLifterManager::DAG->getVTList(MVT::i32);

  // Compute new pointer value
  SDValue ptr =
      ARMLifterManager::DAG->getNode(MathOpc, SL, VTList, Ptr, Offset);

  if (UpdateRef) {
    ARMLifterManager::DAG->ReplaceAllUsesOfValueWith(
        SDValue(Offset.getNode(), 0), ptr);
  }

  return ptr;
}

SDValue StoreLifter::create_store(SDNode *N, SDValue Val, SDValue Chain,
                                  SDValue Ptr, unsigned *ImmSum) {
  llvm::errs() << "Create store\n";

  SDLoc SL(N);

  EVT LdType = N->getValueType(0);

  Value *NullPtr = 0;

  *ImmSum += 4;

  MachineMemOperand *MMO = new MachineMemOperand(
      MachinePointerInfo(NullPtr, *ImmSum), MachineMemOperand::MOStore, 4, 0);

  SDValue ResNode = ARMLifterManager::DAG->getStore(Chain, SL, Val, Ptr, MMO);

  return ResNode;
}

void StoreLifter::clean_graph(SDNode *N, SDValue Chain, SDValue Ptr) {
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
