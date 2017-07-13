//===- ARMInvISelDAG.cpp - Interface for ARM InvISelDAG  =========-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// This file takes code found in LLVM and modifies it.
//
//===----------------------------------------------------------------------===//
//
// Provides inverse instruction selector functionality for the ARM targets.
//
//===----------------------------------------------------------------------===//

#include "Target/ARM/ARMInvISelDAG.h"
#include "ARMBaseInfo.h"

#include "Target/ARM/ARMLifterManager.h"

using namespace llvm;

namespace fracture {

SDNode *ARMInvISelDAG::Transmogrify(SDNode *N) {

  ARMLifterManager::DAG = CurDAG;
  ARMLifterManager::Dec = (Decompiler*)Dec;

  // Insert fixups here
  if (!N->isMachineOpcode()) {
    // Drop noreg registers
    if (N->getOpcode() == ISD::CopyFromReg) {
      const RegisterSDNode *R = dyn_cast<RegisterSDNode>(N->getOperand(1));
      if (R != NULL && R->getReg() == 0) {
        SDValue Tmp = CurDAG->getUNDEF(R->getValueType(0));
        CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Tmp);
        CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), N->getOperand(0));
      }
    }

    // FIXME: This is wrong, originally it returns NULL.
    // I think we need to add it to the CurDAG (only if it hasn't
    // already been added to the CurDAG...)
    return N; // already selected
  }

  uint16_t TargetOpc = N->getMachineOpcode();

  outs() << "[ARMInvIselDAG] Next opcode : " << TargetOpc << "\n\n";

  switch (TargetOpc) {
  default:

    errs() << "[ARMInvIselDAG] Unsupported instruction : " << TargetOpc << "\n";
    exit(1);
    break;

  /* Process Add Instructions */
  case ARM::tADDrr:
  case ARM::tADDhirr:
  case ARM::tADDrSPi:
  case ARM::tADDspi:
  case ARM::tADDi8:
  case ARM::tADDframe:
  case ARM::tADDi3:
  case ARM::tADDrSP:
  case ARM::tADDspr:
  case ARM::t2ADDSri:
  case ARM::t2ADDSrr:
  case ARM::t2ADDSrs:
  case ARM::t2ADDri:
  case ARM::t2ADDri12:
  case ARM::t2ADDrr:
  case ARM::t2ADDrs:
  case ARM::t2ADCri:
    return ARMLifterManager::resolve("ADD")->select(N);
  break;

    /* Process Address Instructions */

    /* Process Subtract Instructions */
    case ARM::RSBrr:
    case ARM::tSUBi3:
    case ARM::tSUBspi:
    case ARM::tSUBi8:
      return ARMLifterManager::resolve("SUB")->select(N);
    break;

    /* Process Paallel Arithmetic Instructions */

    /* Process Saturate Instructions */

    /* Process Multiply Instructions */

    /* Process Divide Instructions */

    /* Process Mode Data Instructions */
    //XXX: Pattern ?
    // t2MOVCCasr	= 2438,
    // t2MOVCCi	= 2439,
    // t2MOVCCi16	= 2440,
    // t2MOVCCi32imm	= 2441,
    // t2MOVCClsl	= 2442,
    // t2MOVCClsr	= 2443,
    // t2MOVCCr	= 2444,
    // t2MOVCCror	= 2445,
    // t2MOVSsi	= 2446,
    // t2MOVSsr	= 2447,
    // t2MOVTi16	= 2448,
    // t2MOVTi16_ga_pcrel	= 2449,
    // t2MOV_ga_pcrel	= 2450,
    // t2MOVi	= 2451,
    // t2MOVi16_ga_pcrel	= 2453,
    // t2MOVi32imm	= 2454,
    // t2MOVr	= 2455,
    // t2MOVsi	= 2456,
    // t2MOVsr	= 2457,
    // t2MOVsra_flag	= 2458,
    // t2MOVsrl_flag	= 2459,

    case ARM::t2MOVi16:
    case ARM::t2MOVi:
    case ARM::t2MVNi:
    case ARM::tMOVSr:
    case ARM::tMOVi8:
    case ARM::tMOVr:
    case ARM::MOVr:
    case ARM::MOVi:
      return ARMLifterManager::resolve("MOVE")->select(N);
    break;

    /* Process Shift Instructions */
    case ARM::tLSLri :
      return ARMLifterManager::resolve("SHIFT")->select(N);
    break;

    /* Process Compare Instructions */
    case ARM::tCMPr:
    case ARM::tCMPi8:
    case ARM::CMPrr:
    case ARM::CMPri:
      return ARMLifterManager::resolve("COMPARE")->select(N);
    break;

    /* Process Logial Instructions */

    /* Process Bit Field Instructions */

    /* Process Pack Instructions */

    /* Process Signed Extend Instructions */

    /* Process Unsigned Extend Instructions */

    /* Process Signed Extend With Add Instructions */

    /* Process Unsigned Extend With Add Instructions */

    /* Process Reverse Instructions */

    /* Process Select Instructions */

    /* Process IfThen Instructions */

    /* Process Branch Instructions */
    case ARM::tBX_RET:
      return ARMLifterManager::resolve("BRANCH")->select(N);

    /* Process Move To Or From PSR Instructions */

    /* Process Processor State Change Instructions */

    /* Process Loard Instructions */
    case ARM::tPOP:
    case ARM::tLDRspi:
    case ARM::t2LDR_POST:
    case ARM::t2LDMIA_UPD :
    case ARM::LDMIA_UPD:
    case ARM::LDMIB_UPD:
    case ARM::LDMDA_UPD:
    case ARM::LDMDB_UPD:
    case ARM::LDMIA:
    case ARM::LDMIB:
    case ARM::LDMDA:
    case ARM::LDMDB:
    case ARM::t2LDRi12:
    case ARM::t2LDR_PRE:
      return ARMLifterManager::resolve("LOAD")->select(N);
    break;

    /* Process Store Instructions */
    case ARM::t2STRBi12:
    case ARM::t2STRi8:
    case ARM::tPUSH:
    case ARM::t2STMDB_UPD:
    case ARM::STR_PRE_IMM:
    case ARM::STRH_POST:
    case ARM::STRD_POST:
    case ARM::t2STR_POST:
    case ARM::tSTRspi:
    case ARM::tSTRi:
    case ARM::STRi12:
    case ARM::t2STMIA:
    case ARM::STMIA:
    case ARM::STMIB:
    case ARM::STMDA:
    case ARM::STMDB:
    case ARM::STMIA_UPD:
    case ARM::STMIB_UPD:
    case ARM::STMDA_UPD:
    case ARM::STMDB_UPD:
    case ARM::t2STRDi8:
    case ARM::t2STRHi12:
    case ARM::t2STRi12:
    case ARM::t2STMIA_UPD:
    case ARM::t2STR_PRE:
    case ARM::t2STMDB:
    case ARM::tSTRr:
      return ARMLifterManager::resolve("STORE")->select(N);
    break;

    /* Process Coprocessor Instructions */

    /* Process Miscellaneous Instructions */

    /* Process Add Instructions */
  }
}

bool ARMInvISelDAG::SelectImmShifterOperand(SDValue N, SDValue &BaseReg,
                                            SDValue &Opc,
                                            bool CheckProfitability) {
  return true;
}

bool ARMInvISelDAG::SelectRegShifterOperand(SDValue N, SDValue &BaseReg,
                                            SDValue &ShReg, SDValue &Opc,
                                            bool CheckProfitability) {
  return true;
}

bool ARMInvISelDAG::SelectAddrModeImm12(SDValue N, SDValue &Base,
                                        SDValue &OffImm) {
  // N is the first operand, but needs to be turned into ISD::ADD
  // Base should be the same as N
  // OffImm should be the Immediate offset
  assert(N.getNode() == Base.getNode() &&
         "Node to replace does not match base!");

  Base = CurDAG->getNode(ISD::ADD, SDLoc(N.getNode()), N.getValueType(), Base,
                         OffImm);
  return true;
}

bool ARMInvISelDAG::SelectLdStSOReg(SDValue N, SDValue &Base, SDValue &Offset,
                                    SDValue &Opc) {
  return true;
}

//-----

AddrMode2Type ARMInvISelDAG::SelectAddrMode2Worker(SDValue N, SDValue &Base,
                                                   SDValue &Offset,
                                                   SDValue &Opc) {
  return AM2_SHOP;
}

bool ARMInvISelDAG::SelectAddrMode2OffsetReg(SDNode *Op, SDValue N,
                                             SDValue &Offset, SDValue &Opc) {
  return true;
}

bool ARMInvISelDAG::SelectAddrMode2OffsetImmPre(SDNode *Op, SDValue N,
                                                SDValue &Offset, SDValue &Opc) {
  return false;
}

bool ARMInvISelDAG::SelectAddrMode2OffsetImm(SDNode *Op, SDValue N,
                                             SDValue &Offset, SDValue &Opc) {
  return false;
}

bool ARMInvISelDAG::SelectAddrOffsetNone(SDValue N, SDValue &Base) {
  Base = N;
  return true;
}

bool ARMInvISelDAG::SelectAddrMode3(SDValue N, SDValue &Base, SDValue &Offset,
                                    SDValue &Opc) {
  return true;
}

bool ARMInvISelDAG::SelectAddrMode3Offset(SDNode *Op, SDValue N,
                                          SDValue &Offset, SDValue &Opc) {
  return true;
}

bool ARMInvISelDAG::SelectAddrMode5(SDValue N, SDValue &Base, SDValue &Offset) {
  return true;
}

bool ARMInvISelDAG::SelectAddrMode6(SDNode *Parent, SDValue N, SDValue &Addr,
                                    SDValue &Align) {
  return true;
}

bool ARMInvISelDAG::SelectAddrMode6Offset(SDNode *Op, SDValue N,
                                          SDValue &Offset) {
  return true;
}

bool ARMInvISelDAG::SelectAddrModePC(SDValue N, SDValue &Offset,
                                     SDValue &Label) {
  return false;
}

//===----------------------------------------------------------------------===//
//                         Thumb Addressing Modes
//===----------------------------------------------------------------------===//

bool ARMInvISelDAG::SelectThumbAddrModeRR(SDValue N, SDValue &Base,
                                          SDValue &Offset) {
  if (N.getOpcode() != ISD::ADD && !CurDAG->isBaseWithConstantOffset(N)) {
    ConstantSDNode *NC = dyn_cast<ConstantSDNode>(N);
    if (!NC || !NC->isNullValue())
      return false;

    Base = Offset = N;
    return true;
  }

  Base = N.getOperand(0);
  Offset = N.getOperand(1);
  return true;
}

bool ARMInvISelDAG::SelectThumbAddrModeRI(SDValue N, SDValue &Base,
                                          SDValue &Offset, unsigned Scale) {
  if (Scale == 4) {
    SDValue TmpBase, TmpOffImm;
    if (SelectThumbAddrModeSP(N, TmpBase, TmpOffImm))
      return false; // We want to select tLDRspi / tSTRspi instead.

    if (N.getOpcode() == ARMISD::Wrapper &&
        N.getOperand(0).getOpcode() == ISD::TargetConstantPool)
      return false; // We want to select tLDRpci instead.
  }

  if (!CurDAG->isBaseWithConstantOffset(N))
    return false;

  // Thumb does not have [sp, r] address mode.
  RegisterSDNode *LHSR = dyn_cast<RegisterSDNode>(N.getOperand(0));
  RegisterSDNode *RHSR = dyn_cast<RegisterSDNode>(N.getOperand(1));
  if ((LHSR && LHSR->getReg() == ARM::SP) ||
      (RHSR && RHSR->getReg() == ARM::SP))
    return false;

  // FIXME: Why do we explicitly check for a match here and then return false?
  // Presumably to allow something else to match, but shouldn't this be
  // documented?
  // int RHSC;
  // if (isScaledConstantInRange(N.getOperand(1), Scale, 0, 32, RHSC))
  //   return false;

  Base = N.getOperand(0);
  Offset = N.getOperand(1);
  return true;
}

bool ARMInvISelDAG::SelectThumbAddrModeRI5S1(SDValue N, SDValue &Base,
                                             SDValue &Offset) {
  return SelectThumbAddrModeRI(N, Base, Offset, 1);
}

bool ARMInvISelDAG::SelectThumbAddrModeRI5S2(SDValue N, SDValue &Base,
                                             SDValue &Offset) {
  return SelectThumbAddrModeRI(N, Base, Offset, 2);
}

bool ARMInvISelDAG::SelectThumbAddrModeRI5S4(SDValue N, SDValue &Base,
                                             SDValue &Offset) {
  return SelectThumbAddrModeRI(N, Base, Offset, 4);
}

bool ARMInvISelDAG::SelectThumbAddrModeImm5S(SDValue N, unsigned Scale,
                                             SDValue &Base, SDValue &OffImm) {
  if (Scale == 4) {
    SDValue TmpBase, TmpOffImm;
    if (SelectThumbAddrModeSP(N, TmpBase, TmpOffImm))
      return false; // We want to select tLDRspi / tSTRspi instead.

    if (N.getOpcode() == ARMISD::Wrapper &&
        N.getOperand(0).getOpcode() == ISD::TargetConstantPool)
      return false; // We want to select tLDRpci instead.
  }

  if (!CurDAG->isBaseWithConstantOffset(N)) {
    // if (N.getOpcode() == ARMISD::Wrapper &&
    //     !(Subtarget->useMovt() &&
    //       N.getOperand(0).getOpcode() == ISD::TargetGlobalAddress)) {
    //   Base = N.getOperand(0);
    // } else {
    Base = N;
    // }

    OffImm = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }

  RegisterSDNode *LHSR = dyn_cast<RegisterSDNode>(N.getOperand(0));
  RegisterSDNode *RHSR = dyn_cast<RegisterSDNode>(N.getOperand(1));
  if ((LHSR && LHSR->getReg() == ARM::SP) ||
      (RHSR && RHSR->getReg() == ARM::SP)) {
    ConstantSDNode *LHS = dyn_cast<ConstantSDNode>(N.getOperand(0));
    ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1));
    unsigned LHSC = LHS ? LHS->getZExtValue() : 0;
    unsigned RHSC = RHS ? RHS->getZExtValue() : 0;

    // Thumb does not have [sp, #imm5] address mode for non-zero imm5.
    if (LHSC != 0 || RHSC != 0)
      return false;

    Base = N;
    OffImm = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }

  // If the RHS is + imm5 * scale, fold into addr mode.
  // int RHSC;
  // if (isScaledConstantInRange(N.getOperand(1), Scale, 0, 32, RHSC)) {
  //   Base = N.getOperand(0);
  //   OffImm = CurDAG->getTargetConstant(RHSC, MVT::i32);
  //   return true;
  // }

  Base = N.getOperand(0);
  OffImm = CurDAG->getTargetConstant(0, MVT::i32);
  return true;
}

bool ARMInvISelDAG::SelectThumbAddrModeImm5S4(SDValue N, SDValue &Base,
                                              SDValue &OffImm) {
  return SelectThumbAddrModeImm5S(N, 4, Base, OffImm);
}

bool ARMInvISelDAG::SelectThumbAddrModeImm5S2(SDValue N, SDValue &Base,
                                              SDValue &OffImm) {
  return SelectThumbAddrModeImm5S(N, 2, Base, OffImm);
}

bool ARMInvISelDAG::SelectThumbAddrModeImm5S1(SDValue N, SDValue &Base,
                                              SDValue &OffImm) {
  return SelectThumbAddrModeImm5S(N, 1, Base, OffImm);
}

bool ARMInvISelDAG::SelectThumbAddrModeSP(SDValue N, SDValue &Base,
                                          SDValue &OffImm) {
  if (N.getOpcode() == ISD::FrameIndex) {
    int FI = cast<FrameIndexSDNode>(N)->getIndex();
    Base = CurDAG->getTargetFrameIndex(FI, TLI->getPointerTy());
    OffImm = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }

  if (!CurDAG->isBaseWithConstantOffset(N))
    return false;

  RegisterSDNode *LHSR = dyn_cast<RegisterSDNode>(N.getOperand(0));
  if (N.getOperand(0).getOpcode() == ISD::FrameIndex ||
      (LHSR && LHSR->getReg() == ARM::SP)) {
    // If the RHS is + imm8 * scale, fold into addr mode.
    // int RHSC;
    // if (isScaledConstantInRange(N.getOperand(1), /*Scale=*/4, 0, 256, RHSC))
    // {
    //   Base = N.getOperand(0);
    //   if (Base.getOpcode() == ISD::FrameIndex) {
    //     int FI = cast<FrameIndexSDNode>(Base)->getIndex();
    //     Base = CurDAG->getTargetFrameIndex(FI, TLI.getPointerTy());
    //   }
    //   OffImm = CurDAG->getTargetConstant(RHSC, MVT::i32);
    //   return true;
    // }
  }

  return false;
}

//===----------------------------------------------------------------------===//
//                        Thumb 2 Addressing Modes
//===----------------------------------------------------------------------===//

bool ARMInvISelDAG::SelectT2ShifterOperandReg(SDValue N, SDValue &BaseReg,
                                              SDValue &Opc) {
  // ARM_AM::ShiftOpc ShOpcVal = ARM_AM::getShiftOpcForNode(N.getOpcode());

  // // Don't match base register only case. That is matched to a separate
  // // lower complexity pattern with explicit register operand.
  // if (ShOpcVal == ARM_AM::no_shift) return false;

  // BaseReg = N.getOperand(0);
  // unsigned ShImmVal = 0;
  // if (ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1))) {
  //   ShImmVal = RHS->getZExtValue() & 31;
  //   Opc = getI32Imm(ARM_AM::getSORegOpc(ShOpcVal, ShImmVal));
  //   return true;
  // }

  return false;
}

bool ARMInvISelDAG::SelectT2AddrModeImm12(SDValue N, SDValue &Base,
                                          SDValue &OffImm) {
  // Match simple R + imm12 operands.

  // Base only.
  if (N.getOpcode() != ISD::ADD && N.getOpcode() != ISD::SUB &&
      !CurDAG->isBaseWithConstantOffset(N)) {
    if (N.getOpcode() == ISD::FrameIndex) {
      // Match frame index.
      int FI = cast<FrameIndexSDNode>(N)->getIndex();
      Base = CurDAG->getTargetFrameIndex(FI, TLI->getPointerTy());
      OffImm = CurDAG->getTargetConstant(0, MVT::i32);
      return true;
    }

    // if (N.getOpcode() == ARMISD::Wrapper &&
    //            !(Subtarget->useMovt() &&
    //              N.getOperand(0).getOpcode() == ISD::TargetGlobalAddress)) {
    //   Base = N.getOperand(0);
    //   if (Base.getOpcode() == ISD::TargetConstantPool)
    //     return false;  // We want to select t2LDRpci instead.
    // } else
    Base = N;
    OffImm = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }

  if (ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1))) {
    if (SelectT2AddrModeImm8(N, Base, OffImm))
      // Let t2LDRi8 handle (R - imm8).
      return false;

    int RHSC = (int)RHS->getZExtValue();
    if (N.getOpcode() == ISD::SUB)
      RHSC = -RHSC;

    if (RHSC >= 0 && RHSC < 0x1000) { // 12 bits (unsigned)
      Base = N.getOperand(0);
      if (Base.getOpcode() == ISD::FrameIndex) {
        int FI = cast<FrameIndexSDNode>(Base)->getIndex();
        Base = CurDAG->getTargetFrameIndex(FI, TLI->getPointerTy());
      }
      OffImm = CurDAG->getTargetConstant(RHSC, MVT::i32);
      return true;
    }
  }

  // Base only.
  Base = N;
  OffImm = CurDAG->getTargetConstant(0, MVT::i32);
  return true;
}

bool ARMInvISelDAG::SelectT2AddrModeImm8(SDValue N, SDValue &Base,
                                         SDValue &OffImm) {
  // Match simple R - imm8 operands.
  if (N.getOpcode() != ISD::ADD && N.getOpcode() != ISD::SUB &&
      !CurDAG->isBaseWithConstantOffset(N))
    return false;

  if (ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1))) {
    int RHSC = (int)RHS->getSExtValue();
    if (N.getOpcode() == ISD::SUB)
      RHSC = -RHSC;

    if ((RHSC >= -255) && (RHSC < 0)) { // 8 bits (always negative)
      Base = N.getOperand(0);
      if (Base.getOpcode() == ISD::FrameIndex) {
        int FI = cast<FrameIndexSDNode>(Base)->getIndex();
        Base = CurDAG->getTargetFrameIndex(FI, TLI->getPointerTy());
      }
      OffImm = CurDAG->getTargetConstant(RHSC, MVT::i32);
      return true;
    }
  }

  return false;
}

bool ARMInvISelDAG::SelectT2AddrModeImm8Offset(SDNode *Op, SDValue N,
                                               SDValue &OffImm) {
  return false;
}

bool ARMInvISelDAG::SelectT2AddrModeSoReg(SDValue N, SDValue &Base,
                                          SDValue &OffReg, SDValue &ShImm) {
  return true;
}

//===--------------------------------------------------------------------===//

/// getAL - Returns a ARMCC::AL immediate node.
static inline SDValue getAL(SelectionDAG *CurDAG) {
  return CurDAG->getTargetConstant((uint64_t)ARMCC::AL, MVT::i32);
}

SDNode *ARMInvISelDAG::SelectARMIndexedLoad(SDNode *N) {
  LoadSDNode *LD = cast<LoadSDNode>(N);
  ISD::MemIndexedMode AM = LD->getAddressingMode();
  if (AM == ISD::UNINDEXED)
    return NULL;
  EVT LoadedVT = LD->getMemoryVT();
  SDValue Offset, AMOpc;
  bool isPre = (AM == ISD::PRE_INC) || (AM == ISD::PRE_DEC);
  unsigned Opcode = 0;
  bool Match = false;
  if (LoadedVT == MVT::i32 && isPre &&
      SelectAddrMode2OffsetImmPre(N, LD->getOffset(), Offset, AMOpc)) {
    Opcode = ARM::LDR_PRE_IMM;
    Match = true;
  } else if (LoadedVT == MVT::i32 && !isPre &&
             SelectAddrMode2OffsetImm(N, LD->getOffset(), Offset, AMOpc)) {
    Opcode = ARM::LDR_POST_IMM;
    Match = true;
  } else if (LoadedVT == MVT::i32 &&
             SelectAddrMode2OffsetReg(N, LD->getOffset(), Offset, AMOpc)) {
    Opcode = isPre ? ARM::LDR_PRE_REG : ARM::LDR_POST_REG;
    Match = true;

  } else if (LoadedVT == MVT::i16 &&
             SelectAddrMode3Offset(N, LD->getOffset(), Offset, AMOpc)) {
    Match = true;
    Opcode = (LD->getExtensionType() == ISD::SEXTLOAD)
                 ? (isPre ? ARM::LDRSH_PRE : ARM::LDRSH_POST)
                 : (isPre ? ARM::LDRH_PRE : ARM::LDRH_POST);
  } else if (LoadedVT == MVT::i8 || LoadedVT == MVT::i1) {
    if (LD->getExtensionType() == ISD::SEXTLOAD) {
      if (SelectAddrMode3Offset(N, LD->getOffset(), Offset, AMOpc)) {
        Match = true;
        Opcode = isPre ? ARM::LDRSB_PRE : ARM::LDRSB_POST;
      }
    } else {
      if (isPre &&
          SelectAddrMode2OffsetImmPre(N, LD->getOffset(), Offset, AMOpc)) {
        Match = true;
        Opcode = ARM::LDRB_PRE_IMM;
      } else if (!isPre &&
                 SelectAddrMode2OffsetImm(N, LD->getOffset(), Offset, AMOpc)) {
        Match = true;
        Opcode = ARM::LDRB_POST_IMM;
      } else if (SelectAddrMode2OffsetReg(N, LD->getOffset(), Offset, AMOpc)) {
        Match = true;
        Opcode = isPre ? ARM::LDRB_PRE_REG : ARM::LDRB_POST_REG;
      }
    }
  }

  if (Match) {
    if (Opcode == ARM::LDR_PRE_IMM || Opcode == ARM::LDRB_PRE_IMM) {
      SDValue Chain = LD->getChain();
      SDValue Base = LD->getBasePtr();
      SDValue Ops[] = {Base, AMOpc, getAL(CurDAG),
                       CurDAG->getRegister(0, MVT::i32), Chain};
      return CurDAG->getMachineNode(Opcode, SDLoc(N), MVT::i32, MVT::i32,
                                    MVT::Other, Ops);
    } else {
      SDValue Chain = LD->getChain();
      SDValue Base = LD->getBasePtr();
      SDValue Ops[] = {
          Base, Offset, AMOpc, getAL(CurDAG), CurDAG->getRegister(0, MVT::i32),
          Chain};
      return CurDAG->getMachineNode(Opcode, SDLoc(N), MVT::i32, MVT::i32,
                                    MVT::Other, Ops);
    }
  }

  return NULL;
}

SDNode *ARMInvISelDAG::SelectT2IndexedLoad(SDNode *N) {
  LoadSDNode *LD = cast<LoadSDNode>(N);
  ISD::MemIndexedMode AM = LD->getAddressingMode();
  if (AM == ISD::UNINDEXED)
    return NULL;

  EVT LoadedVT = LD->getMemoryVT();
  bool isSExtLd = LD->getExtensionType() == ISD::SEXTLOAD;
  SDValue Offset;
  bool isPre = (AM == ISD::PRE_INC) || (AM == ISD::PRE_DEC);
  unsigned Opcode = 0;
  bool Match = false;
  if (SelectT2AddrModeImm8Offset(N, LD->getOffset(), Offset)) {
    switch (LoadedVT.getSimpleVT().SimpleTy) {
    case MVT::i32:
      Opcode = isPre ? ARM::t2LDR_PRE : ARM::t2LDR_POST;
      break;
    case MVT::i16:
      if (isSExtLd)
        Opcode = isPre ? ARM::t2LDRSH_PRE : ARM::t2LDRSH_POST;
      else
        Opcode = isPre ? ARM::t2LDRH_PRE : ARM::t2LDRH_POST;
      break;
    case MVT::i8:
    case MVT::i1:
      if (isSExtLd)
        Opcode = isPre ? ARM::t2LDRSB_PRE : ARM::t2LDRSB_POST;
      else
        Opcode = isPre ? ARM::t2LDRB_PRE : ARM::t2LDRB_POST;
      break;
    default:
      return NULL;
    }
    Match = true;
  }

  if (Match) {
    SDValue Chain = LD->getChain();
    SDValue Base = LD->getBasePtr();
    SDValue Ops[] = {Base, Offset, getAL(CurDAG),
                     CurDAG->getRegister(0, MVT::i32), Chain};
    return CurDAG->getMachineNode(Opcode, SDLoc(N), MVT::i32, MVT::i32,
                                  MVT::Other, Ops);
  }

  return NULL;
}

bool ARMInvISelDAG::SelectCMOVPred(SDValue N, SDValue &Pred, SDValue &Reg) {
  const ConstantSDNode *CN = cast<ConstantSDNode>(N);
  Pred = CurDAG->getTargetConstant(CN->getZExtValue(), MVT::i32);
  Reg = CurDAG->getRegister(ARM::CPSR, MVT::i32);
  return true;
}

} // namespace fracture
