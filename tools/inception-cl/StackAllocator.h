#ifndef STACK_ALLOCATOR_H
#define STACK_ALLOCATOR_H

class StackAllocator {
 public:
  StackAllocator() {}

  ~StackAllocator() {}

  static void Allocate(llvm::Module* mod) {
    Value* Reg = mod->getGlobalVariable("STACK");

    if (Reg == NULL) {
      Type* Ty = ArrayType::get(IntegerType::get(mod->getContext(), 4), 16400);

      Constant* Initializer = Constant::getNullValue(Ty);

      GlobalVariable* gvar_ptr_SP =
          new GlobalVariable(*mod,   // Module
                             Ty,     // Type
                             false,  // isConstant
                             GlobalValue::CommonLinkage, Initializer, "STACK");
      gvar_ptr_SP->setAlignment(4);
    }
  }
};
#endif
