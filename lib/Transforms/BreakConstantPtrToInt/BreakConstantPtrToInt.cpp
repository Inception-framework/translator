
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

//===- BreakConstantPtrToInt.cpp - Change constant GEPs into GEP instructions - --//
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

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"

#include "Transforms/BreakConstantPtrToInt.h"

#include <iostream>
#include <map>
#include <utility>

using namespace llvm;

namespace {

//
// Pass: BreakConstantPtrToInt
//
// Description:
//  This pass modifies a function so that it uses GEP instructions instead of
//  GEP constant expressions.
//
struct BreakConstantPtrToInt : public FunctionPass {
  static char ID;

  BreakConstantPtrToInt() : FunctionPass(ID) {}

  // const char *getPassName() const { return "Remove Constant GEP Expressions";
  // }

  virtual bool runOnFunction(Function &F);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    // This pass does not modify the control-flow graph of the function
    AU.setPreservesCFG();
  }
};
}  // end anonymous namespace

// Identifier variable for the pass
char BreakConstantPtrToInt::ID = 42;

// Register the pass
static RegisterPass<BreakConstantPtrToInt> P(
    "break-constptrtoint", "Remove PtrToInt Constant Expressions");

FunctionPass *llvm::createBreakConstantPtrToIntPass() {
  return new BreakConstantPtrToInt();
}

// Statistics
// STATISTIC(GEPChanges, "Number of Converted GEP Constant Expressions");
// STATISTIC(TotalChanges, "Number of Converted Constant Expressions");

//
// Function: hasConstantPtrToInt()
//
// Description:
//  This function determines whether the given value is a constant expression
//  that has a constant GEP expression embedded within it.
//
// Inputs:
//  V - The value to check.
//
// Return value:
//  NULL  - This value is not a constant expression with a constant expression
//          GEP within it.
//  ~NULL - A pointer to the value casted into a ConstantExpr is returned.
//
static ConstantExpr *hasConstantPtrToInt(Value *V) {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
    if (CE->getOpcode() == Instruction::PtrToInt) {
      return CE;
    } else if (CE->getOpcode() == Instruction::BitCast) {
      return CE;
    } else {
      for (unsigned index = 0; index < CE->getNumOperands(); ++index) {
        if (hasConstantPtrToInt(CE->getOperand(index))) return CE;
      }
    }
  }

  return 0;
}

//
// Function: convertGEP()
//
// Description:
//  Convert a GEP constant expression into a GEP instruction.
//
// Inputs:
//  CE       - The GEP constant expression.
//  InsertPt - The instruction before which to insert the new GEP instruction.
//
// Return value:
//  A pointer to the new GEP instruction is returned.
//
static Instruction *convertGEP(ConstantExpr *CE, Instruction *InsertPt) {
  //
  // Create iterators to the indices of the constant expression.
  //
  std::vector<Value *> Indices;
  for (unsigned index = 1; index < CE->getNumOperands(); ++index) {
    Indices.push_back(CE->getOperand(index));
  }

  //
  // Update the statistics.
  //
  // ++GEPChanges;

  //
  // Make the new GEP instruction.
  //
  return (GetElementPtrInst::CreateInBounds(CE->getOperand(0), Indices,
                                            CE->getName(), InsertPt));
}

//
// Function: convertExpression()
//
// Description:
//  Convert a constant expression into an instruction.  This routine does *not*
//  perform any recursion, so the resulting instruction may have constant
//  expression operands.
//
static Instruction *convertExpression(ConstantExpr *CE, Instruction *InsertPt) {
  //
  // Convert this constant expression into a regular instruction.
  //
  Instruction *NewInst = 0;
  switch (CE->getOpcode()) {
    case Instruction::GetElementPtr: {
      NewInst = convertGEP(CE, InsertPt);
      break;
    }

    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
    case Instruction::UDiv:
    case Instruction::SDiv:
    case Instruction::FDiv:
    case Instruction::URem:
    case Instruction::SRem:
    case Instruction::FRem:
    case Instruction::Shl:
    case Instruction::LShr:
    case Instruction::AShr:
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor: {
      Instruction::BinaryOps Op = (Instruction::BinaryOps)(CE->getOpcode());
      NewInst = BinaryOperator::Create(Op, CE->getOperand(0), CE->getOperand(1),
                                       CE->getName(), InsertPt);
      break;
    }

    case Instruction::Trunc:
    case Instruction::ZExt:
    case Instruction::SExt:
    case Instruction::FPToUI:
    case Instruction::FPToSI:
    case Instruction::UIToFP:
    case Instruction::SIToFP:
    case Instruction::FPTrunc:
    case Instruction::FPExt:
    case Instruction::PtrToInt:
    case Instruction::IntToPtr:
    case Instruction::BitCast: {
      Instruction::CastOps Op = (Instruction::CastOps)(CE->getOpcode());
      NewInst = CastInst::Create(Op, CE->getOperand(0), CE->getType(),
                                 CE->getName(), InsertPt);
      break;
    }

    case Instruction::FCmp:
    case Instruction::ICmp: {
      Instruction::OtherOps Op = (Instruction::OtherOps)(CE->getOpcode());
      NewInst = CmpInst::Create(Op, CE->getPredicate(), CE->getOperand(0),
                                CE->getOperand(1), CE->getName(), InsertPt);
      break;
    }

    case Instruction::Select:
      NewInst = SelectInst::Create(CE->getOperand(0), CE->getOperand(1),
                                   CE->getOperand(2), CE->getName(), InsertPt);
      break;

    case Instruction::ExtractElement:
    case Instruction::InsertElement:
    case Instruction::ShuffleVector:
    case Instruction::InsertValue:
    default:
      assert(0 && "Unhandled constant expression!\n");
      break;
  }

  //
  // Update the statistics.
  //
  // ++TotalChanges;

  return NewInst;
}

//
// Method: runOnFunction()
//
// Description:
//  Entry point for this LLVM pass.
//
// Return value:
//  true  - The function was modified.
//  false - The function was not modified.
//
bool BreakConstantPtrToInt::runOnFunction(Function &F) {
  bool modified = false;

  // Worklist of values to check for constant GEP expressions
  std::vector<Instruction *> Worklist;

  //
  // Initialize the worklist by finding all instructions that have one or more
  // operands containing a constant GEP expression.
  //
  for (Function::iterator BB = F.begin(); BB != F.end(); ++BB) {
    for (BasicBlock::iterator i = BB->begin(); i != BB->end(); ++i) {
      //
      // Scan through the operands of this instruction.  If it is a constant
      // expression GEP, insert an instruction GEP before the instruction.
      //
      Instruction *I = i;
      for (unsigned index = 0; index < I->getNumOperands(); ++index) {
        if (hasConstantPtrToInt(I->getOperand(index))) {
          Worklist.push_back(I);
        }
      }
    }
  }

  //
  // Determine whether we will modify anything.
  //
  if (Worklist.size()) modified = true;

  //
  // While the worklist is not empty, take an item from it, convert the
  // operands into instructions if necessary, and determine if the newly
  // added instructions need to be processed as well.
  //
  while (Worklist.size()) {
    Instruction *I = Worklist.back();
    Worklist.pop_back();

    //
    // Scan through the operands of this instruction and convert each into an
    // instruction.  Note that this works a little differently for phi
    // instructions because the new instruction must be added to the
    // appropriate predecessor block.
    //
    if (PHINode *PHI = dyn_cast<PHINode>(I)) {
      for (unsigned index = 0; index < PHI->getNumIncomingValues(); ++index) {
        //
        // For PHI Nodes, if an operand is a constant expression with a GEP, we
        // want to insert the new instructions in the predecessor basic block.
        //
        // Note: It seems that it's possible for a phi to have the same
        // incoming basic block listed multiple times; this seems okay as long
        // the same value is listed for the incoming block.
        //
        Instruction *InsertPt = PHI->getIncomingBlock(index)->getTerminator();
        if (ConstantExpr *CE =
                hasConstantPtrToInt(PHI->getIncomingValue(index))) {
          Instruction *NewInst = convertExpression(CE, InsertPt);
          for (unsigned i2 = index; i2 < PHI->getNumIncomingValues(); ++i2) {
            if ((PHI->getIncomingBlock(i2)) == PHI->getIncomingBlock(index))
              PHI->setIncomingValue(i2, NewInst);
          }
          Worklist.push_back(NewInst);
        }
      }
    } else {
      for (unsigned index = 0; index < I->getNumOperands(); ++index) {
        //
        // For other instructions, we want to insert instructions replacing
        // constant expressions immediently before the instruction using the
        // constant expression.
        //
        if (ConstantExpr *CE = hasConstantPtrToInt(I->getOperand(index))) {
          Instruction *NewInst = convertExpression(CE, I);
          I->replaceUsesOfWith(CE, NewInst);
          Worklist.push_back(NewInst);
        }
      }
    }
  }

  return modified;
}
