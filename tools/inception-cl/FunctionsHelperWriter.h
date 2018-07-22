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

#ifndef FUNCTION_HELPER_WRITER_H
#define FUNCTION_HELPER_WRITER_H

#include "CodeInv/Decompiler.h"
#include "CodeInv/Disassembler.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/CFG.h"
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
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <algorithm>
#include "llvm/IR/Metadata.h"

typedef enum FUNCTION_HELPER {
  DUMP_STACK = 0,
  DUMP_REGISTERS = 1,
  INTERRUPT_PROLOGUE = 2,
  INTERRUPT_EPILOGUE = 3,
  ICP = 4,
  INTERRUPT_HANDLER = 5,
  SWITCH_SP = 6,
  WRITEBACK_SP = 7,
  CACHE_SP = 8,
} FUNCTION_HELPER;

typedef enum FHW_POSITION { BEGIN = 0, END = 1, NONE = 2 } FHW_POSITION;

class FunctionsHelperWriter {
 protected:
  FunctionsHelperWriter(){};
  ~FunctionsHelperWriter(){};

  static void FNHDumpStack(llvm::Module* mod, llvm::Instruction* before);

  static void FNHDumpRegisters(llvm::Module* mod, llvm::Instruction* before);

  static void FNHInterruptPrologue(llvm::Module* mod,
                                   llvm::Instruction* before);

  static void FNHInterruptEpilogue(llvm::Module* mod,
                                   llvm::Instruction* before);

  static void FNHICP(llvm::Module* mod, llvm::Instruction* inst);

  static void FNHInterruptHandler(llvm::Module* mod, llvm::Instruction* inst);

  static void FNHSwitchSP(llvm::Module* mod, llvm::Instruction* inst);

  static void FNHWritebackSP(llvm::Module* mod, llvm::Instruction* inst);

  static void FNHCacheSP(llvm::Module* mod, llvm::Instruction* inst);

  static llvm::Instruction* GetBegin(llvm::Function* func);

  static llvm::Instruction* GetLast(llvm::Function* func);

 public:
  static void Write(FHW_POSITION position, FUNCTION_HELPER type,
                    llvm::Module* mod, llvm::Function* func,
                    llvm::Instruction* before = NULL);
};

#endif
