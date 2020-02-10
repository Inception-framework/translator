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

#ifndef STACK_ALLOCATOR_H
#define STACK_ALLOCATOR_H

#include "Utils/Builder.h"

class StackAllocator {
 public:
  StackAllocator() {}

  ~StackAllocator() {}

  static void Allocate(llvm::Module* mod, const Disassembler* Dis) {
    // Main stack
    const object::SectionRef Section_s = Dis->getSectionByName(".stack");
    //if( Section_s.getObject() == NULL ) {
    if( Section_s.getSize() > 0x800 ) {
      Type* Ty_s = ArrayType::get(IntegerType::get(mod->getContext(), 4),10000);
      Reg(".stack", Ty_s);
    } else {
      Type* Ty_s = ArrayType::get(IntegerType::get(mod->getContext(), 4),
                                Section_s.getSize());
      Reg(".stack", Ty_s);
    }

    // User stack allocated by SectionsWriter
  }

  // XXX: SP cannot be initialized with other than 0
  // XXX: Therefore, We introduce a store before the first
  // instruction
  static void InitSP(llvm::Module* mod, const Disassembler* Dis) {
    // Create registers
    Value* CONTROL_1 = Reg("CONTROL_1");  // CONTROL[1]
    Value* MSP = Reg("MSP");              // Main Stack Pointer
    Value* PSP = Reg("PSP");              // Process Stack Pointer
    Value* SP = Reg("SP");                // Stack Pointer

    // CONTROL_1 = 0 by default, i.e. SP = MSP

    // Initialise MSP and SP to the main stack, assume that PSP will be
    // initialised by the code
    const object::SectionRef Section = Dis->getSectionByName(".stack");
    unsigned sp = Section.getAddress() + (Section.getSize() / 2) - 4;
    ConstantInt* c_sp;
    c_sp = ConstantInt::get(mod->getContext(), APInt(32, sp));
    Function* main_fct = mod->getFunction("main");
    Instruction* inst = main_fct->getEntryBlock().begin();
    IRBuilder<>* builder = new IRBuilder<>(inst);
    builder->CreateStore(c_sp, MSP);
    builder->CreateStore(c_sp, SP);
  }
};
#endif
