#ifndef STACK_ALLOCATOR_H
#define STACK_ALLOCATOR_H

#include "Utils/Builder.h"

class StackAllocator {
 public:
  StackAllocator() {}

  ~StackAllocator() {}

  static void Allocate(llvm::Module* mod, const Disassembler* Dis) {
    // Main stack
    const object::SectionRef Section = Dis->getSectionByName(".stack");
    Type* Ty = ArrayType::get(IntegerType::get(mod->getContext(), 4),
                              Section.getSize());
    Reg(".stack", Ty);
  }

  // XXX: SP cannot be initialized with other than 0
  // XXX: Therefore, We introduce a store before the first
  // instruction
  static void InitSP(llvm::Module* mod, const Disassembler* Dis) {
    const object::SectionRef Section = Dis->getSectionByName(".stack");

    unsigned sp = Section.getAddress() + (Section.getSize() / 2) - 4;
    ConstantInt* c_sp;

    c_sp = ConstantInt::get(mod->getContext(), APInt(32, sp));

    Function* main_fct = mod->getFunction("main");
    Instruction* inst = main_fct->getEntryBlock().begin();

    IRBuilder<>* builder = new IRBuilder<>(inst);
    builder->CreateStore(c_sp, Reg("SP"));
  }
};
#endif
