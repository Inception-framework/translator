#include "Target/ARM/ARMLifterManager.h"

#include "llvm/CodeGen/SelectionDAGNodes.h"

#include "Target/ARM/AddLifter.h"
#include "Target/ARM/BranchLifter.h"
#include "Target/ARM/CompareLifter.h"
#include "Target/ARM/ExtendLifter.h"
#include "Target/ARM/FlagsLifter.h"
#include "Target/ARM/LoadLifter.h"
#include "Target/ARM/LogicalLifter.h"
#include "Target/ARM/MoveDataLifter.h"
#include "Target/ARM/ShiftLifter.h"
#include "Target/ARM/StoreLifter.h"
#include "Target/ARM/SubtractLifter.h"
#include "Target/ARM/SVCallLifter.h"
#include "Target/ARM/CoprocLifter.h"

#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

ARMLifterManager::~ARMLifterManager() {}

ARMLifterManager::ARMLifterManager() {
  /*
   * All the architecture specific lifters should be declared here
   */
  lifters.insert(
      std::pair<std::string, ARMLifter*>("ADD", new AddLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("BRANCH", new BranchLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("SUB", new SubtractLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("MOVE", new MoveDataLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("SHIFT", new ShiftLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("COMPARE", new CompareLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("LOAD", new LoadLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("STORE", new StoreLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("LOGICAL", new LogicalLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("FLAGS", new FlagsLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("SVCALL", new SVCallLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("EXTEND", new ExtendLifter(this)));

  lifters.insert(
      std::pair<std::string, ARMLifter*>("COPROC", new CoprocLifter(this)));

  registerAll();
}

/*
 * This function select the correct Lifter for a giving opcode
 */
LifterSolver* ARMLifterManager::resolve(unsigned opcode) {
  auto search = solver.find(opcode);

  if (search != solver.end()) {
    LifterSolver* solver = search->second;

    return solver;
  } else {
    return NULL;
  }
}

void ARMLifterManager::registerAll() {
  for (auto b = lifters.begin(), e = lifters.end(); b != e; b++) {
    llvm::outs() << "Registering Lifter : " << b->first << "\n";
    b->second->registerLifter();
  }
}

void ARMLifterManager::registerLifter(ARMLifter* lifter,
                                      std::string lifter_name, unsigned opcode,
                                      LifterHandler handler) {
  LifterSolver* lifter_solver = new LifterSolver(lifter, lifter_name, handler);

  solver.insert(std::pair<unsigned, LifterSolver*>(opcode, lifter_solver));
}

/*
 * This function select the correct Lifter for a giving name
 */
ARMLifter* ARMLifterManager::resolve(StringRef name) {
  auto search = lifters.find(name);

  if (search != lifters.end()) {
    ARMLifter* lifter = search->second;

    return lifter;
  } else {
    return NULL;
  }
}
