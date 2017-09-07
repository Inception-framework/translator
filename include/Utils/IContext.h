#ifndef INCEPTION_CONTEXT_H
#define INCEPTION_CONTEXT_H

#include "Utils/ErrorHandling.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/Module.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "Target/ARM/ARMLifterManager.h"

using namespace llvm;
using namespace inception;

class ARMLifterManager;

class IContext {
 public:
  static IContext* m_instance;

  static LLVMContext* getContext();

  static LLVMContext& getContextRef();

  static llvm::Module* Mod;

  static const TargetRegisterInfo* RegisterInfo;

  static DenseMap<const SDNode*, Value*> VisitMap;

  static ARMLifterManager *alm;

};

#endif
