#include "Target/ARM/ARMLifterManager.h"

#include "Target/ARM/AddLifter.h"
#include "Target/ARM/BranchLifter.h"
#include "Target/ARM/MoveDataLifter.h"
#include "Target/ARM/SubtractLifter.h"
#include "Target/ARM/ShiftLifter.h"
#include "Target/ARM/CompareLifter.h"

llvm::SelectionDAG *ARMLifterManager::DAG = NULL;

std::map<std::string, ARMLifter *> ARMLifterManager::managers = {
    {"ADD", new AddLifter()},
    {"BRANCH", new BranchLifter()},
    {"MOVE", new MoveDataLifter()},
    {"SUB", new MoveDataLifter()},
    {"SHIFT", new MoveDataLifter()},
    {"COMPARE", new MoveDataLifter()}
  };

ARMLifter *ARMLifterManager::resolve(std::string domain) {
  auto search = ARMLifterManager::managers.find(domain);
  if (search != ARMLifterManager::managers.end()) {
    return search->second;
  } else {
    return NULL;
  }
}
