#ifndef FUNCTION_HELPER_WRITER_H
#define FUNCTION_HELPER_WRITER_H

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
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <algorithm>
#include "llvm/IR/Metadata.h"

typedef enum FUNCTION_HELPER {
  INIT_STACK = 0,
  DUMP_REGISTERS = 1
} FUNCTION_HELPER;

typedef enum FHW_POSITION { BEGIN = 0, END = 1, NONE = 2 } FHW_POSITION;

class FunctionsHelperWriter {
 protected:
  FunctionsHelperWriter(){};
  ~FunctionsHelperWriter(){};

  static void FNHInitStack(llvm::Module* mod, llvm::Instruction* before);

  static void FNHDumpRegisters(llvm::Module* mod, llvm::Instruction* before);

  static llvm::Instruction* GetBegin(llvm::Module* mod);

  static llvm::Instruction* GetLast(llvm::Module* mod);

 public:
  static void Write(FHW_POSITION position, FUNCTION_HELPER type,
                    llvm::Module* mod, llvm::Instruction* before = NULL);
};

#endif