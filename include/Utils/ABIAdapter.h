#include "Builder.h"
#include "IContext.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

class ABIAdapter {
 public:
  ABIAdapter();

  ~ABIAdapter();

  Value* getNext(IRBuilder<>* IRB);

  Value* Higher(Function* Func, IRBuilder<>* IRB);

  Value* Lower(Value* inst, IRBuilder<>* IRB);

  Value* Higher(Type* Ty, IRBuilder<>* IRB);

 private:
  unsigned index;

  std::vector<std::string> destinations;

  Value* sp;

  Value* HigherCollections(Type* Ty, unsigned size, IRBuilder<>* IRB);

  Value* HigherInteger(Type* Ty, IRBuilder<>* IRB);

  Value* HigherFloat(Type* type, IRBuilder<>* IRB);

  Value* HigherPointer(Type* Ty, IRBuilder<>* IRB);

  Value* LowerCollections(Value* collection, unsigned size, IRBuilder<>* IRB);

  Value* LowerInteger(Value* integer, IRBuilder<>* IRB);

  Value* LowerPointer(Value* pointer, IRBuilder<>* IRB);
};
