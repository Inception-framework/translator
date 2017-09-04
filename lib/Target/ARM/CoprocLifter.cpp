#include "Target/ARM/CoprocLifter.h"

#include "ARMBaseInfo.h"
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

}

void CoprocLifter::MRSHandler(SDNode *N, IRBuilder<> *IRB) {
  Value* Res = NULL;
  SDNode* Op0 = N->getOperand(0).getNode();

  ConstantSDNode* SrcNode = dyn_cast<ConstantSDNode>(Op0);
  if (SrcNode == NULL) {
    llvm::errs() << "[CoprocLifter] DoMSR Handler expected ConstantSDNode ...";
    return;
  }

  int64_t reg_id = SrcNode->getZExtValue();

  /*
  * Return SP value instead of PSP/MSP
  * TODO: Add support for PSP/MSP
  */
  switch(reg_id) {
    case 0: //APSR
    Res = ReadReg(Reg("APSR"), IRB, 32);
    break;
    case 3: //PSR
    Res = ReadReg(Reg("PSR"), IRB, 32);
    break;
    case 5: //IPSR
    Res = ReadReg(Reg("IPSR"), IRB, 32);
    break;
    case 6: //EPSR
    Res = ReadReg(Reg("EPSR"), IRB, 32);
    break;
    case 8: //PSP
    // Res = ReadReg(Reg("PSP"), IRB, 32);
    case 9: //MSP
    // Res = ReadReg(Reg("MSP"), IRB, 32);
    Res = ReadReg(Reg("SP"), IRB, 32);
    break;
    case 16: //PRIMASK
    Res = ReadReg(Reg("PRIMASK"), IRB, 32);
    break;
    case 17: //BASEPRI
    Res = ReadReg(Reg("BASEPRI"), IRB, 32);
    break;
    case 19: //FAULTMASK
    Res = ReadReg(Reg("FAULTMASK"), IRB, 32);
    break;
    case 20: //CONTROL
    Res = ReadReg(Reg("CONTROL"), IRB, 32);
    break;
    default :
      llvm::errs() << "MRSHandler do not know source coproc register\n";
      exit(0);
  }

  saveNodeValue(N, Res);
}
