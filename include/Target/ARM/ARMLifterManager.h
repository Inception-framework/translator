#ifndef ARM_LIFTER_MANAGER_H
#define ARM_LIFTER_MANAGER_H

#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#include "CodeInv/Decompiler.h"
#include "Target/ARM/ARMLifter.h"

#include <iostream>
#include <map>
#include <string>
#include <tuple>

namespace fracture {
class Decompiler;
}

class ARMLifter;

#ifndef LIFTER_HANDLER_H
#define LIFTER_HANDLER_H
typedef void (ARMLifter::*LifterHandler)(llvm::SDNode* N, llvm::IRBuilder<>* IRB);
#endif

typedef struct LifterSolver {
  LifterHandler handler;

  std::string name;

  ARMLifter* lifter;

  LifterSolver(ARMLifter* _lifter, std::string _name, LifterHandler _handler)
      : lifter(_lifter), name(_name), handler(_handler){};
} LifterSolver;

class ARMLifterManager {
 public:
  ~ARMLifterManager();

  ARMLifterManager();

  // Return the coresponding architecture dependent Lifter for the specified
  // opcode
  LifterSolver* resolve(unsigned opcode);

  void registerLifter(ARMLifter* lifter, std::string name, unsigned opcode, LifterHandler handler);

  void registerAll();

  ARMLifter* resolve(StringRef name);

  const TargetRegisterInfo* RegisterInfo;

  llvm::Module* Mod;

  IndexedMap<Value*> RegMap;

  DenseMap<const SDNode*, Value*> VisitMap;

  StringMap<StringRef> BaseNames;

  llvm::SelectionDAG *DAG;

  fracture::Decompiler* Dec;

 private:
  std::map<unsigned, LifterSolver*> solver;

  std::map<std::string, ARMLifter*> lifters;
};

#endif
