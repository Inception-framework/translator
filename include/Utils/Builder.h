#include "IContext.h"
#include "Target/ARM/ARMBaseInfo.h"
#include "Utils/IContext.h"
#include "Target/ARM/ARMBaseInfo.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Target/TargetRegisterInfo.h"

#ifndef BUILDER_H
#define BUILDER_H

using namespace llvm;
using namespace fracture;

void initAPI(llvm::Module* _Mod, Decompiler* DEC);

std::string getReg(const SDNode* N);

SDNode* LookUpSDNode(SDNode* N, std::string name);

bool IsCPSR(SDNode* N);

bool IsPC(SDNode* N);

bool IsITSTATE(SDNode* N);

bool IsSigned(SDNode* N);

bool IsSetFlags(SDNode* N);

Value* Bool2Int(Value* v, IRBuilder<>* IRB);

Value* Reg(StringRef name);

Value* getConstant(StringRef value);

Value* getConstant(uint32_t value);

Value* ReadAddress(Value* Rd, Type* Ty, IRBuilder<>* IRB);

Value* ReadReg(Value* Rn, IRBuilder<>* IRB, int Width);

Value* ReadReg(Value* Rn, IRBuilder<>* IRB);

Value* WriteReg(Value* Rn, Value* Rd, IRBuilder<>* IRB, int Width,
                bool extend);

Value* WriteReg(Value* Rn, Value* Rd, IRBuilder<>* IRB);

Value* WriteReg(Value* Rn, Value* Rd, IRBuilder<>* IRB, int Width);

Value* UpdateRd(Value* Rn, Value* Offset, IRBuilder<>* IRB, bool Increment);

void saveNodeValue(const SDNode* N, Value* Rn);

Value* getSavedValue(const SDNode* N);

llvm::Module* getModule();

Value* getInteger(int value);

std::string getRegisterSDNodeName(const RegisterSDNode* R);

Value* createCondition(int cond, IRBuilder<>* IRB);

#endif
