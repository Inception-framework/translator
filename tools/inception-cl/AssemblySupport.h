#ifndef ASSEMBLY_SUPPORT_H
#define ASSEMBLY_SUPPORT_H

#include "CodeInv/Disassembler.h"
#include "CodeInv/FractureMemoryObject.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

class AssemblySupport {
 public:
  static void ImportAll(llvm::Module* mod, const Disassembler* Dis) {
    uint64_t SymAddr;
    llvm::StringRef SymName;
    object::SymbolRef::Type SymbolTy;
    std::error_code ec;

    for (object::symbol_iterator I = Dis->getExecutable()->symbols().begin(),
                                 E = Dis->getExecutable()->symbols().end();
         I != E; ++I) {
      if ((ec = I->getType(SymbolTy))) {
        errs() << ec.message() << "\n";
        continue;
      }

      if (SymbolTy != object::SymbolRef::ST_Function) {
        continue;
      }

      if ((ec = I->getAddress(SymAddr))) {
        errs() << ec.message() << "\n";
        continue;
      }

      if ((ec = I->getName(SymName))) {
        errs() << ec.message() << "\n";
        continue;
      }

      std::string FName = Dis->getFunctionName(SymAddr);

      Function* Func = mod->getFunction(FName);
      if (Func == NULL) {
        Import(SymName, mod);
      }
    }
  }

  static void Import(llvm::StringRef name, llvm::Module* mod) {
    FunctionType* FType = FunctionType::get(
        Type::getPrimitiveType(mod->getContext(), Type::VoidTyID), false);

    mod->getOrInsertFunction(name, FType);
  }
};

#endif
