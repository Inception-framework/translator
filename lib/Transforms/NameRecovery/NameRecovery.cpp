
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

//===--- NameRecovery - recovers function parameters and locals -*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This function pass looks at LLVM IR from the Decompiler and tries to
// recover the function parameter types and return type of the function.
//
// Works only at the function level. It may be necessary to make this global
// across the entire decompiled program output.
//
// NOTE: Does not do anything useful yet. Placeholder for now.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: January 15, 2014
//===----------------------------------------------------------------------===//

#include "Transforms/NameRecovery.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"

#include <iostream>
#include <map>
#include <string>
#include <tuple>

#include <list>

using namespace llvm;

namespace {

struct NameRecovery : public FunctionPass {
  static char ID;
  NameRecovery() : FunctionPass(ID) {}

  virtual bool runOnFunction(Function& F);

  // We don't modify the program, so we preserve all analyses
  virtual void getAnalysisUsage(AnalysisUsage& AU) const {
    AU.setPreservesAll();
  }

  llvm::StringRef getIndexedValueName(llvm::StringRef BaseName, Module* Mod);

  llvm::StringRef getBaseValueName(llvm::StringRef BaseName);

  llvm::StringRef getInstructionName(Instruction* I);

  IndexedMap<Value*> RegMap;

  DenseMap<const SDNode*, Value*> VisitMap;

  StringMap<StringRef> BaseNames;

};  // end of struct NameRecovery

}  // end anonymous namespace

char NameRecovery::ID = 0;

static RegisterPass<NameRecovery> X("NameRecovery", "Name recovery pass",
                                    false /* Only looks at CFG */,
                                    false /* Analysis Pass */);

FunctionPass* llvm::createNameRecoveryPass() { return new NameRecovery(); }

bool NameRecovery::runOnFunction(Function& F) {
  if (F.isDeclaration()) {
    return false;
  }

  for (Function::iterator BB = F.begin(), BBE = F.end(); BB != BBE; ++BB) {
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
      Instruction* Instruction = &*I;

      Module* Mod = F.getParent();

      // If there is any direct pointer to a glabal registers as operand, we can
      // retrieve the name from the base name of this operand. Otherwise we need
      // to check SSA declaration of all operands.
      StringRef IRName = getInstructionName(Instruction);
      if (IRName.equals(StringRef())) continue;

      StringRef Name = getIndexedValueName(IRName, Mod);

      I->setName(Name);
    }
  }

  return false;
}

StringRef NameRecovery::getIndexedValueName(StringRef BaseName, Module* Mod) {
  const ValueSymbolTable& ST = Mod->getValueSymbolTable();

  // In the common case, the name is not already in the symbol table.
  Value* V = ST.lookup(BaseName);
  if (V == NULL) {
    return BaseName;
  }

  // Otherwise, there is a naming conflict.  Rename this value.
  // FIXME: AFAIK this is never deallocated (memory leak). It should be
  // free'd after it gets added to the symbol table (which appears to do a
  // copy as indicated by the original code that stack allocated this
  // variable).
  SmallString<256>* UniqueName =
      new SmallString<256>(BaseName.begin(), BaseName.end());
  unsigned Size = BaseName.size();

  // Add '_' as the last character when BaseName ends in a number
  if (BaseName[Size - 1] <= '9' && BaseName[Size - 1] >= '0') {
    UniqueName->resize(Size + 1);
    (*UniqueName)[Size] = '_';
    Size++;
  }

  unsigned LastUnique = 0;
  while (1) {
    // Trim any suffix off and append the next number.
    UniqueName->resize(Size);
    raw_svector_ostream(*UniqueName) << ++LastUnique;

    // Try insert the vmap entry with this suffix.
    V = ST.lookup(*UniqueName);
    // FIXME: ^^ this lookup does not appear to be working on
    // non-globals... Temporary Fix: check if it has a BaseNames
    // entry
    if (V == NULL && BaseNames[*UniqueName].empty()) {
      BaseNames[*UniqueName] = BaseName;
      return *UniqueName;
    }
  }
}

StringRef NameRecovery::getBaseValueName(StringRef BaseName) {
  // Note: An alternate approach would be to pull the Symbol table and
  // do a string search, but this is much easier to implement.
  StringRef Res = BaseNames.lookup(BaseName);
  if (Res.empty()) {
    return BaseName;
  }
  return Res;
}

StringRef NameRecovery::getInstructionName(Instruction* I) {

  if (isa<ReturnInst>(I) || isa<BranchInst>(I) || isa<SwitchInst>(I) ||
      isa<IndirectBrInst>(I) || isa<ResumeInst>(I) || isa<UnreachableInst>(I) ||
      isa<StoreInst>(I) || isa<FenceInst>(I) || isa<AtomicRMWInst>(I) ||
      isa<AtomicCmpXchgInst>(I) || isa<CallInst>(I) || isa<InvokeInst>(I))
    return StringRef();

  for (uint64_t i = 0; i < I->getNumOperands(); i++) {
    Value* value = I->getOperand(i);

    return getBaseValueName(value->getName());
  }

  return StringRef();
}
