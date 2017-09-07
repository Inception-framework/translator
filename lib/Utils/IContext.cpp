#include "Utils/IContext.h"

using namespace llvm;

IContext* IContext::m_instance = NULL;

DenseMap<const SDNode*, Value*> IContext::VisitMap;

llvm::Module* IContext::Mod;

const TargetRegisterInfo* IContext::RegisterInfo;

ARMLifterManager *IContext::alm = new ARMLifterManager();

LLVMContext* IContext::getContext() { return &(IContext::Mod->getContext()); }

LLVMContext& IContext::getContextRef() { return IContext::Mod->getContext(); }
