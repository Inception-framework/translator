//===--- IRMerger - Merge two IR Modules ----------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class uses SDNodes and emits IR. It is intended to be extended by Target
// implementations who have special ISD legalization nodes.
//
// Author: Corteggiani Nassim <nassim.corteggiani@maximintegrated.com>
// Date: April 19, 2017
//===----------------------------------------------------------------------===//

#ifndef IRMERGER_H
#define IRMERGER_H

#include "CodeInv/Decompiler.h"
#include "CodeInv/Disassembler.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "llvm/IR/Metadata.h"
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <algorithm>

#include <map>
#include <stack>
#include <string>

using namespace llvm;

namespace fracture {

class Decompiler;
class Disassembler;

class IRMerger {
 public:
  IRMerger(Decompiler *DEC, std::string new_function_name);

  ~IRMerger();

  void Merge(std::string old_function_name);

  void SetNewFunction(std::string new_function_name);

  void Run();

  static std::map<std::string, SDNode *> registersNodes;

 protected:
  static bool first_call;

  void CreateADDCarryHelper();

  Function *Decompile();

  StringRef getIndexedValueName(StringRef BaseName);

  StringRef getBaseValueName(StringRef BaseName);

  void MarkOldInstructions();

  void MapArgsToRegs();

  void RemoveUseless();

  void SetReturnType();

  void RemoveInstruction(llvm::Instruction* instruction);

  Function *fct;

  BasicBlock *entry_bb;

  Decompiler *DEC;

  IndexedMap<Value *> RegMap;

  StringRef *function_name;

  std::vector<Instruction *> marked_old_instructions;

  std::vector<Instruction *> marked_old_binstructions;

  std::vector<BasicBlock *> marked_old_basicblocks;
};

}  // end namespace fracture

#endif /* IRMERGER_H */
