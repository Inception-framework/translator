//===--- Decompiler - Decompiles machine basic blocks -----------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class uses the disassembler and inverse instruction selector classes
// to decompile target specific code into LLVM IR and return function objects
// back to the user.
//
//
//
// Copyright (c) 2017 Maxim Integrated, Inc.
// Author: Nassim Corteggiani <nassim.corteggiani@maximintegrated.com>
//
// Copyright (c) 2017 EURECOM, Inc.
// Author: Giovanni Camurati <giovanni.camurati@eurecom.fr>
//
//===----------------------------------------------------------------------===//

#ifndef DECOMPILER_H
#define DECOMPILER_H

#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/PassManager.h"

#include "CodeInv/Disassembler.h"
#include "Transforms/TypeRecovery.h"
#include "Transforms/NameRecovery.h"

using namespace llvm;

namespace fracture {

class Decompiler {
public:

  Decompiler(Disassembler *NewDis, Module *NewMod = NULL,
    raw_ostream &InfoOut = nulls(), raw_ostream &ErrOut = nulls());
  ~Decompiler();

  void printInstructions(formatted_raw_ostream &Out, unsigned Address);

  ///===-------------------------------------------------------------------===//
  /// decompile - decompile starting at a given memory address.
  ///
  /// This function recursively descends the code to decompile the function
  /// and all functions called by this function.
  ///
  /// Results are cached, so if a function has already been decompiled, we refer
  /// to the stored result.
  ///
  /// @param Address - the address to start decompiling.
  ///
  void decompile(unsigned Address);
  Function* decompileFunction(unsigned Address);
  BasicBlock *decompileBasicBlock(MachineBasicBlock *MBB, Function *F,
                                  unsigned Address, unsigned entryAddress);

  uint64_t getFunctionAddress(Function *F);
  BasicBlock* getOrCreateBasicBlock(unsigned Address, Function *F);
  BasicBlock* getOrCreateBasicBlock(StringRef BBName, Function *F);

  void sortBasicBlock(BasicBlock *BB);
  void splitBasicBlockIntoBlock(Function::iterator Src,
                                BasicBlock::iterator FirstInst,
                                BasicBlock *Tgt);

  SelectionDAG* createDAGFromMachineBasicBlock(MachineBasicBlock *MBB);

  uint64_t getBasicBlockAddress(BasicBlock *BB);

  SelectionDAG* getCurrentDAG() { return DAG; }
  const Disassembler* getDisassembler() const { return Dis; }
  void setViewMCDAGs(bool Setting) { ViewMCDAGs = Setting; }
  Module* getModule() { return Mod; }
  LLVMContext* getContext() { return Context; }

  uint32_t it_state;
  uint32_t it_true;
  bool it_start;

 private:
  Disassembler *Dis;
  Module *Mod;
  LLVMContext *Context;
  SelectionDAG* DAG;
  bool ViewMCDAGs;

  void printSDNode(std::map<SDValue, std::string> &OpMap,
    std::stack<SDNode *> &NodeStack, SDNode *CurNode, SelectionDAG *DAG);
  void printDAG(SelectionDAG *DAG);

  void checkAddrInSection(unsigned Address);
  Function *getFunctionFromMF(MachineFunction *MF);
  void decompileBasicBlocks(MachineFunction *MF, Function *F, unsigned Address,
                            unsigned entryAddress);
  void handleInBetweenBasicBlocks(Function *F, unsigned Address);

  /// Error printing
  raw_ostream &Infos, &Errs;
  void printInfo(std::string Msg) const {
    Infos << "Disassembler: " << Msg << "\n";
  }
  void printError(std::string Msg) const {
    Errs << "Disassembler: " << Msg << "\n";
    Errs.flush();
  }
};

} // end namespace fracture

#endif /* DECOMPILER_H */
