#ifndef STACK_ALLOCATOR_H
#define STACK_ALLOCATOR_H

class StackAllocator {
 public:
  StackAllocator() {}

  ~StackAllocator() {}

  static void Allocate(llvm::Module* mod, const Disassembler* Dis) {
    const object::SectionRef Section = Dis->getSectionByName(".stack");

    Value* Reg = mod->getGlobalVariable(".stack");

    if (Reg == NULL) {
      Type* Ty = ArrayType::get(IntegerType::get(mod->getContext(), 4),
                                Section.getSize());

      Constant* Initializer = Constant::getNullValue(Ty);

      GlobalVariable* gvar_ptr_SP =
          new GlobalVariable(*mod,   // Module
                             Ty,     // Type
                             false,  // isConstant
                             GlobalValue::CommonLinkage, Initializer, ".stack");
      gvar_ptr_SP->setAlignment(4);
    }
  }

  // XXX: SP cannot be initialized with other than 0
  // XXX: Therefore, We introduce a store before the first
  // instruction
  static void InitSP(llvm::Module* mod, const Disassembler* Dis) {
    const object::SectionRef Section = Dis->getSectionByName(".stack");

    unsigned sp = Section.getAddress() + (Section.getSize() / 2);
    ConstantInt* c_sp;

    Value* Reg = mod->getGlobalVariable("SP");
    if (Reg == NULL) {
      Type* Ty = IntegerType::get(mod->getContext(), 32);

      Constant* Initializer = Constant::getNullValue(Ty);

      Reg = new GlobalVariable(*mod,   // Module
                               Ty,     // Type
                               false,  // isConstant
                               GlobalValue::CommonLinkage, Initializer, "SP");
    }

    c_sp = ConstantInt::get(mod->getContext(), APInt(32, sp));

    Function* main_fct = mod->getFunction("main");
    Instruction* inst = main_fct->getEntryBlock().begin();

    IRBuilder<>* builder = new IRBuilder<>(inst);
    builder->CreateStore(c_sp, Reg);
  }
};
#endif
