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
    Type* Ty_s = ArrayType::get(IntegerType::get(mod->getContext(), 4),
                                Section_s.getSize());
    Reg(".stack", Ty_s);

    // User stack
    const object::SectionRef Section_us = Dis->getSectionByName(".user_stack");
    Type* Ty_us = ArrayType::get(IntegerType::get(mod->getContext(), 4),
                                 Section_us.getSize());
    Reg(".user_stack", Ty_us);
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
