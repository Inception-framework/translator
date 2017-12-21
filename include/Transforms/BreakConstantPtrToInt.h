//===- BreakConstantPtrToInt.h - Change constant GEPs into GEP instructions --- --//
//
//                          The SAFECode Compiler
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass changes all GEP constant expressions into GEP instructions.  This
// permits the rest of SAFECode to put run-time checks on them if necessary.
//
//===----------------------------------------------------------------------===//

#ifndef BREAKCONSTANTPTRTOINT_H
#define BREAKCONSTANTPTRTOINT_H

#include "llvm/IR/Dominators.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"


namespace llvm {

class FunctionPass;
class Pass;

FunctionPass* createBreakConstantPtrToIntPass();

} // End namespace llvm

#endif
