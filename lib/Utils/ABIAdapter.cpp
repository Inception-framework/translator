#include "Utils/ABIAdapter.h"

using namespace llvm;

ABIAdapter::ABIAdapter() {
  destinations.push_back("R0");
  destinations.push_back("R1");
  destinations.push_back("R2");
  destinations.push_back("R3");

  index = 0;

  sp = NULL;
}

ABIAdapter::~ABIAdapter() {}

Value* ABIAdapter::getNext(IRBuilder<>* IRB) {
  if (index < destinations.size()) {
    return Reg(destinations.at(index++));
  } else {
    if (sp == NULL)
      sp = ReadReg(Reg("SP"), IRB);
    else
      sp = UpdateRd(sp, getConstant(4), IRB, true);
    return ReadReg(sp, IRB);
  }
}

Value* ABIAdapter::Higher(Function* Func, IRBuilder<>* IRB) {
  std::vector<Value*> Args;
  std::vector<Type*> ArgTypes;

  for (Function::arg_iterator I = Func->arg_begin(), E = Func->arg_end();
       I != E; ++I) {
    ArgTypes.push_back(I->getType());

    Value* Res = Higher(I->getType(), IRB);

    Args.push_back(Res);
  }

  Value* Call = IRB->CreateCall(dyn_cast<Value>(Func), Args);

  return Call;
}

Value* ABIAdapter::HigherCollections(Type* Ty, unsigned size,
                                     IRBuilder<>* IRB) {
  Value* array = IRB->CreateAlloca(Ty);

  for (unsigned int i = 0; i < size; i++) {
    Value* IdxList[2];
    IdxList[0] = getConstant("0");
    IdxList[1] = ConstantInt::get(IContext::getContextRef(), APInt(32, i, 10));

    Value* ptr = IRB->CreateGEP(array, IdxList);

    Value* Reg = ReadReg(getNext(IRB), IRB);

    IRB->CreateStore(Reg, ptr);
  }
  Value* Res = IRB->CreateLoad(array);

  return Res;
}

Value* ABIAdapter::HigherInteger(Type* type, IRBuilder<>* IRB) {
  Value* Res = ReadReg(getNext(IRB), IRB);

  Res = IRB->CreateTrunc(Res, type);

  return Res;
}

Value* ABIAdapter::HigherPointer(Type* Ty, IRBuilder<>* IRB) {
  Value* Res = ReadReg(getNext(IRB), IRB);

  Type* i32_ptr = IntegerType::get(IContext::getContextRef(), 32);

  Res = IRB->CreateIntToPtr(Res, i32_ptr->getPointerTo());

  Res = ReadReg(Res, IRB);

  Res = IRB->CreateIntToPtr(Res, Ty);

  return Res;
}

Value* ABIAdapter::Higher(Type* Ty, IRBuilder<>* IRB) {
  unsigned num_elements;

  switch (Ty->getTypeID()) {
    case llvm::Type::StructTyID: {
      num_elements = Ty->getStructNumElements();
      Type* Ty = StructType::get(
          IntegerType::get(IContext::getContextRef(), 32), num_elements);

      return HigherCollections(Ty, num_elements, IRB);
      break;
    }
    case llvm::Type::VectorTyID: {
      num_elements = Ty->getVectorNumElements();
      Type* Ty = VectorType::get(
          IntegerType::get(IContext::getContextRef(), 32), num_elements);

      return HigherCollections(Ty, num_elements, IRB);
      break;
    }
    case llvm::Type::ArrayTyID: {
      num_elements = Ty->getArrayNumElements();
      Type* Ty = ArrayType::get(IntegerType::get(IContext::getContextRef(), 32),
                                num_elements);

      return HigherCollections(Ty, num_elements, IRB);
      break;
    }
    case llvm::Type::IntegerTyID: {
      return HigherInteger(Ty, IRB);
      break;
    }
    case llvm::Type::VoidTyID: {
      return getConstant(0);
      break;
    }
    case llvm::Type::PointerTyID: {
      return HigherPointer(Ty, IRB);
      break;
    }
    case llvm::Type::HalfTyID:
    case llvm::Type::FloatTyID:
    case llvm::Type::DoubleTyID:
    case llvm::Type::X86_FP80TyID:
    case llvm::Type::FP128TyID:
    case llvm::Type::PPC_FP128TyID:
    case llvm::Type::LabelTyID:
    case llvm::Type::MetadataTyID:
    case llvm::Type::X86_MMXTyID:
    case llvm::Type::FunctionTyID: {
      return NULL;
      break;
    }
    default: {
      return NULL;
      break;
    }
  }
}

Value* ABIAdapter::LowerCollections(Value* collection, unsigned size,
                                    IRBuilder<>* IRB) {
  // unsigned reg_counter = 0;
  // Value* res = NULL;

  // for (uint64_t i = 0; i < size; i++) {
  //   Value* reg = Reg(StringRef("R" + std::to_string(reg_counter)));
  //
  //   Value* IdxList[2];
  //   IdxList[0] = getConstant("0");
  //   IdxList[1] = ConstantInt::get(IContext::getContextRef(), APInt(32, i,
  //   10));
  //
  //   Value* ptr = IRB->CreateGEP(collection, IdxList);
  //
  //   // Value* element = IRB->CreateExtractValue(collection, i);
  //   res = ReadReg(ptr, IRB);
  //   res = WriteReg(res, reg, IRB);
  //
  //   reg_counter++;
  // }
  return collection;
}

Value* ABIAdapter::LowerInteger(Value* integer, IRBuilder<>* IRB) {
  integer =
      IRB->CreateZExt(integer, IntegerType::get(IContext::getContextRef(), 32));
  return WriteReg(integer, getNext(IRB), IRB);
}

Value* ABIAdapter::LowerPointer(Value* pointer, IRBuilder<>* IRB) {
  pointer = IRB->CreatePtrToInt(
      pointer, IntegerType::get(IContext::getContextRef(), 32));
  return WriteReg(pointer, getNext(IRB), IRB);
}

Value* ABIAdapter::Lower(Value* inst, IRBuilder<>* IRB) {
  Type* ReturnType = inst->getType();

  unsigned num_elements;

  switch (ReturnType->getTypeID()) {
    case llvm::Type::StructTyID: {
      num_elements = ReturnType->getStructNumElements();
      return LowerCollections(inst, num_elements, IRB);
      break;
    }
    case llvm::Type::VectorTyID: {
      num_elements = ReturnType->getVectorNumElements();
      return LowerCollections(inst, num_elements, IRB);
      break;
    }
    case llvm::Type::ArrayTyID: {
      num_elements = ReturnType->getArrayNumElements();
      return LowerCollections(inst, num_elements, IRB);
      break;
    }
    case llvm::Type::IntegerTyID: {
      return LowerInteger(inst, IRB);
      break;
    }
    case llvm::Type::VoidTyID: {
      return inst;
      break;
    }
    case llvm::Type::PointerTyID: {
      return LowerPointer(inst, IRB);
      break;
    }
    case llvm::Type::HalfTyID:
    case llvm::Type::FloatTyID:
    case llvm::Type::DoubleTyID:
    case llvm::Type::X86_FP80TyID:
    case llvm::Type::FP128TyID:
    case llvm::Type::PPC_FP128TyID:
    case llvm::Type::LabelTyID:
    case llvm::Type::MetadataTyID:
    case llvm::Type::X86_MMXTyID:
    case llvm::Type::FunctionTyID: {
      return NULL;
      break;
    }
    default: {
      return NULL;
      break;
    }
  }
}
