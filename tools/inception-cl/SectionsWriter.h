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

#ifndef SECTION_WRITER_H
#define SECTION_WRITER_H

#include "CodeInv/Decompiler.h"
#include "CodeInv/Disassembler.h"
#include "CodeInv/FractureMemoryObject.h"
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

#include "Utils/Builder.h"

using namespace fracture;
using namespace llvm;

class SectionsWriter {
 public:
  static void WriteSection(StringRef SectionName, const Disassembler* Dis,
                           Module* mod) {
    // Locals
    ConstantInt *c_addr, *constant, *c_4;

    if (mod->getGlobalVariable(SectionName) != NULL) {
      inception_error(
          "[SectionsWriter] mod->getGlobalVariable(SectionName) != NULL");
    }

    // Prepare section
    StringRef Bytes;
    const object::SectionRef* Section = NULL;

    for (object::section_iterator si = Dis->getExecutable()->section_begin(),
                                  se = Dis->getExecutable()->section_end();
         si != se; ++si) {
      StringRef Name;
      if (si->getName(Name)) {
        continue;
      }
      if (Name.equals(SectionName)) {
        Section = &*si;
        break;
      }
    }

    if (Section == NULL) {
      inception_warning("[SectionsWriter] Section '%s' not found",
                        SectionName.str().c_str());
      return;
    }

    if (Section->getSize() == 0) {
      inception_warning("[SectionsWriter] Section.getSize()");
      return;
    }

    std::error_code ec = Section->getContents(Bytes);
    if (ec) {
      inception_error("[SectionsWriter] %s", ec.message().c_str());
    }
    unsigned size = Section->getSize();
    FractureMemoryObject* CurSectionMemory =
        new FractureMemoryObject(Bytes, Section->getAddress());

    // Set Insertion point
    Function* fct = mod->getFunction("main");
    BasicBlock* bb = &(fct->getEntryBlock());
    Instruction* inst = &(bb->front());

    inception_message("%s ...", SectionName.str().c_str());

    // IRBuilder to insert new instruction after inst
    IRBuilder<>* IRB = new IRBuilder<>(inst);

    // Declare the data section
    Type* Ty =
        ArrayType::get(IntegerType::get(mod->getContext(), 32), size / 4);
    Constant* Initializer = Constant::getNullValue(Ty);

    GlobalVariable* DataSection = new GlobalVariable(
        /*Module=*/*mod,
        /*Type=*/Ty,
        /*isConstant=*/false,
        /*Linkage=*/GlobalValue::CommonLinkage,
        /*Initializer=*/Initializer,  // has initializer, specified below
        /*Name=*/StringRef(SectionName));
    DataSection->setAlignment(4);

    // We do not need to initialize the heap
    if (SectionName.equals(".heap")) {
      inception_message("%s declared but not initialised",
                        SectionName.str().c_str());
      return;
    }

    c_4 = ConstantInt::get(mod->getContext(), APInt(32, 2));
    Value* R0 = Reg("R0");

    constant = ConstantInt::get(mod->getContext(), APInt(32, 0));

    for (uint64_t i = 0; i < (size / 4); i++) {
      uint64_t Address = i * 4 + CurSectionMemory->getBase();

      uint8_t* B = new uint8_t(4);
      int NumRead = CurSectionMemory->readBytes(B, Address, 4);
      if (NumRead < 0) {
        llvm::errs() << "Unable to read current section memory!\n";
        return;
      }

      uint32_t val = B[0] | (B[1] << 8) | (B[2] << 16) | (B[3] << 24);
      constant = ConstantInt::get(mod->getContext(), APInt(32, val));

      c_addr = ConstantInt::get(mod->getContext(), APInt(32, Address));

      Value* ptr = IRB->CreateIntToPtr(c_addr, R0->getType());

      IRB->CreateStore(constant, ptr);

      ptr = IRB->CreateAdd(c_addr, c_4);
    }
  }
};
#endif
