#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetRegisterInfo.h"

#include "CodeInv/Decompiler.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/IRBuilder.h"

#ifndef ARM_LIFTER_H
#define ARM_LIFTER_H

#include "ARMLifterManager.h"

class ARMLifterManager;
class ARMLifter;

#ifndef LIFTER_HANDLER_H
#define LIFTER_HANDLER_H
typedef void (ARMLifter::*LifterHandler)(llvm::SDNode* N,
                                         llvm::IRBuilder<>* IRB);
#endif

namespace fracture {
class Decompiler;
}

class ARMLifter {
 public:
  virtual void registerLifter() = 0;

  ARMLifter(ARMLifterManager* _alm) : alm(_alm){};

 protected:
  llvm::Value* visit(const llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  // Store handler for each supported opcode
  std::map<unsigned, LifterHandler> solver;

  ARMLifterManager* alm;

  std::string getReg(const SDNode* N);

  llvm::Value* visitRegister(const llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  llvm::Value* visitCopyFromReg(const llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  llvm::Value* visitCopyToReg(const llvm::SDNode* N, llvm::IRBuilder<>* IRB);

  llvm::Value* visitConstant(const llvm::SDNode* N);

  Value* WriteReg(Value* Rn, Value* Rd, IRBuilder<>* IRB, int Width=32);

  Value* ReadReg(Value* Rd, IRBuilder<>* IRB, int Width=32);

  Value* ReadAddress(Value* Rd, Type* Ty, IRBuilder<>* IRB);

  Value* saveNodeValue(SDNode* N, Value* Rn);

  Value* UpdateRd(Value* Rn, Value* Offset, IRBuilder<>* IRB, bool Increment);

  Value* getConstant(StringRef value);

  Value* Reg(StringRef name);

  Value* Bool2Int(Value* v, IRBuilder<>* IRB);

  bool IsSigned(SDNode* N);

  bool IsCPSR(SDNode* N);

  SDNode* LookUpSDNode(SDNode* N, std::string name);
};

#endif
