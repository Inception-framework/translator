
/*
    This file is part of Inception translator.

    Inception translator is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Inception translator.  If not, see <https://www.gnu.org/licenses/>.

    Copyright (c) 2017 Maxim Integrated, Inc.
    Author: Nassim Corteggiani <nassim.corteggiani@maximintegrated.com>

    Copyright (c) 2017 EURECOM, Inc.
    Author: Giovanni Camurati <giovanni.camurati@eurecom.fr>
*/

#include "Target/ARM/CoprocLifter.h"

#include "Target/ARM/ARMBaseInfo.h"
#include "Target/ARM/ARMISD.h"
#include "Target/ARM/ARMLifterManager.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/FlagsLifter.h"

using namespace llvm;
using namespace fracture;

// t2MRS_AR	= 2464,
// t2MRS_M	= 2465,
// t2MRSbanked	= 2466,
// t2MRSsys_AR	= 2467,
// t2MSR_AR	= 2468,
// t2MSR_M	= 2469,
// t2MSRbanked	= 2470,
void CoprocLifter::registerLifter() {
  alm->registerLifter(this, std::string("CoprocLifter"), (unsigned)ARM::t2MSR_M,
                      (LifterHandler)&CoprocLifter::MSRHandler);
  alm->registerLifter(this, std::string("CoprocLifter"), (unsigned)ARM::t2MRS_M,
                      (LifterHandler)&CoprocLifter::MRSHandler);
}

void CoprocLifter::MSRHandler(SDNode *N, IRBuilder<> *IRB) {
  // destination (special register)
  SDNode* Dest = N->getOperand(0).getNode();

  ConstantSDNode* DestNode = dyn_cast<ConstantSDNode>(Dest);
  if (DestNode == NULL) {
    inception_error("[CoprocLifter] MSR Handler expected ConstantSDNode ...");
  }

  int64_t reg_id = DestNode->getZExtValue() & 0xff;

  // source (normal register)
  Value* Rn = visit(N->getOperand(1).getNode(), IRB);

  /*
  * Return SP value instead of PSP/MSP
  * TODO: Add support for PSP/MSP
  */
  Value* Rdest = NULL;
  switch (reg_id) {
    case 9: {  // PSP
      // write remote
      Value* Ret = WriteReg(Rn, Reg("PSP"), IRB);
      // update cache if necessary
      Constant* cache_sp = GetVoidFunctionPointer("inception_cache_sp");
      IRB->CreateCall(cache_sp);
      saveNodeValue(N, Ret);
      return;
    }
    case 8: {  // MSP
      // write remote
      Value* Ret = WriteReg(Rn, Reg("MSP"), IRB);
      // update cache if necessary
      Constant* cache_sp = GetVoidFunctionPointer("inception_cache_sp");
      IRB->CreateCall(cache_sp);
      saveNodeValue(N, Ret);
      return;
    }
    case 0:  // APSR
      Rdest = Reg("APSR");
      inception_warning(
          "[MSRHandler] APSR in not supported, treated as dummy write");
      break;
    case 3:  // PSR
      Rdest = Reg("PSR");
      inception_warning(
          "[MSRHandler] PSR in not supported, treated as dummy write");
      break;
    case 5:  // IPSR
      Rdest = Reg("IPSR");
      inception_warning(
          "[MSRHandler] IPSR in not supported, treated as dummy write");
      break;
    case 6:  // EPSR
      Rdest = Reg("EPSR");
      inception_warning(
          "[MSRHandler] EPSR in not supported, treated as dummy write");
      break;
    case 16:  // PRIMASK
      Rdest = Reg("PRIMASK");
      inception_warning(
          "[MSRHandler] PRIMASK in not supported, treated as dummy write");
      break;
    case 17: {  // BASEPRI
      Rdest = Reg("BASEPRI");
      Constant* write_basepri =
          GetVoidFunctionPointer("inception_write_basepri");
      IRB->CreateCall(write_basepri, Rn);
      break;
    }
    case 19:  // FAULTMASK
      Rdest = Reg("FAULTMASK");
      inception_warning(
          "[MSRHandler] FAULTMASK in not supported, treated as dummy write");
      break;
    case 20: {  // CONTROL
      inception_warning(
          "[MSRHandler] only CONTROL[1] is supported, the rest i  treated as a "
          "dummy write");
      Value* ctrl1_tgt = IRB->CreateLShr(Rn,getConstant(1));
      ctrl1_tgt = IRB->CreateAnd(ctrl1_tgt, getConstant(1));
      Constant* switch_sp = GetIntFunctionPointer("inception_switch_sp");
      IRB->CreateCall(switch_sp, ctrl1_tgt);
      Rdest = Reg("CONTROL_1");
      Rn = ReadReg(Rdest, IRB);
      break;
    }
    default:
      inception_error("[MSRHandler] unsupported coproc register");
  }

  // store in the dest register
  Instruction* Ret = IRB->CreateStore(Rn, Rdest);

  // dummy output
  saveNodeValue(N, Ret);
}

void CoprocLifter::MRSHandler(SDNode *N, IRBuilder<> *IRB) {
  Value* Res = NULL;
  SDNode* Op0 = N->getOperand(0).getNode();

  ConstantSDNode* SrcNode = dyn_cast<ConstantSDNode>(Op0);
  if (SrcNode == NULL) {
    inception_error("[CoprocLifter] MRS Handler expected ConstantSDNode ...");
  }

  int64_t reg_id = SrcNode->getZExtValue();

  /*
  * Return SP value instead of PSP/MSP
  * TODO: Add support for PSP/MSP
  */
  switch(reg_id) {
    case 0: //APSR
    Res = ReadReg(Reg("APSR"), IRB, 32);
    inception_warning(
        "[MRSHandler] APSR in not supported, the read value may be wrong");
    break;
    case 3: //PSR
    Res = ReadReg(Reg("PSR"), IRB, 32);
    inception_warning(
        "[MRSHandler] PSR in not supported, the read value may be wrong");
    break;
    case 5: //IPSR
    Res = ReadReg(Reg("IPSR"), IRB, 32);
    inception_warning(
        "[MRSHandler] IPSR in not supported, the read value may be wrong");
    break;
    case 6: //EPSR
    Res = ReadReg(Reg("EPSR"), IRB, 32);
    inception_warning(
        "[MRSHandler] EPSR in not supported, the read value may be wrong");
    break;
    case 9: {  // PSP
      // write back possibly dirty SP
      Constant* wb_sp = GetVoidFunctionPointer("inception_writeback_sp");
      IRB->CreateCall(wb_sp);
      // read remote
      Res = ReadReg(Reg("PSP"), IRB, 32);
      break;
    }
    case 8: {  // MSP
      // write back possibly dirty SP
      Constant* wb_sp = GetVoidFunctionPointer("inception_writeback_sp");
      IRB->CreateCall(wb_sp);
      // read remote
      Res = ReadReg(Reg("MSP"), IRB, 32);
      break;
    }
    case 16: //PRIMASK
    Res = ReadReg(Reg("PRIMASK"), IRB, 32);
    inception_warning(
        "[MRSHandler] PRIMASK in not supported, the read value may be wrong");
    break;
    case 17: {  // BASEPRI
      Constant* read_basepri = GetVoidFunctionPointer("inception_read_basepri");
      IRB->CreateCall(read_basepri, Reg("BASEPRI"));
      Res = ReadReg(Reg("BASEPRI"), IRB, 32);
      break;
    }
    case 19: //FAULTMASK
    Res = ReadReg(Reg("FAULTMASK"), IRB, 32);
    inception_warning(
        "[MRSHandler] FAULTMASK in not supported, the read value may be wrong");
    break;
    case 20: //CONTROL
      Res = ReadReg(Reg("CONTROL_1"), IRB, 32);
      Res = IRB->CreateShl(Res, getConstant(1));
      inception_message("[MRSHandler] CONTROL only bit 1 is supported");
      break;
    default :
      inception_error("[MRSHandler] unsupported coproc register");
  }

  saveNodeValue(N, Res);
}
