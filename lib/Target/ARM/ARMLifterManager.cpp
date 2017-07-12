#include "Target/ARM/ARMLifterManager.h"

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/AddLifter.h"
#include "Target/ARM/BranchLifter.h"
#include "Target/ARM/MoveDataLifter.h"
#include "Target/ARM/SubtractLifter.h"
#include "Target/ARM/ShiftLifter.h"
#include "Target/ARM/CompareLifter.h"
#include "Target/ARM/LoadLifter.h"
#include "Target/ARM/StoreLifter.h"

using namespace llvm;

llvm::SelectionDAG *ARMLifterManager::DAG = NULL;

fracture::Decompiler *ARMLifterManager::Dec = NULL;

std::map<std::string, ARMLifter *> ARMLifterManager::managers = {
    {"ADD", new AddLifter()},
    {"BRANCH", new BranchLifter()},
    {"MOVE", new MoveDataLifter()},
    {"SUB", new MoveDataLifter()},
    {"SHIFT", new MoveDataLifter()},
    {"COMPARE", new MoveDataLifter()},
    {"LOAD", new LoadLifter()},
    {"STORE", new StoreLifter()}
  };

ARMLifter *ARMLifterManager::resolve(std::string domain) {
  auto search = ARMLifterManager::managers.find(domain);
  if (search != ARMLifterManager::managers.end()) {
    return search->second;
  } else {
    return NULL;
  }
}

/// Moves Op[0] to Op[Op.size()-1]. This is done for certain load/store operands
/// during inverse DAG Selection.
void ARMLifterManager::FixChainOp(SDNode *N) {
  SDValue Chain = N->getOperand(0);
  unsigned NumOps = N->getNumOperands();

  assert(NumOps > 1 && "Not enough operands to swap the Chain on Load/Store!");
  assert(Chain.getValueType() == MVT::Other && "Not a chain value!");

  SmallVector<SDValue, 3> Ops;
  for (unsigned i = 1; i != NumOps; ++i) {
    Ops.push_back(N->getOperand(i));
  }
  Ops.push_back(Chain);

  N = DAG->UpdateNodeOperands(N, Ops);
}
