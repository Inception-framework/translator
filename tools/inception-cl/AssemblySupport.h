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

#include "Utils/IContext.h"

using namespace fracture;
using namespace llvm;

class AssemblySupport {
 public:
  static std::set<std::string> ImportAll(llvm::Module* mod, Disassembler* Dis) {
    uint64_t SymbolAddr;
    llvm::StringRef SymbolName;
    object::SymbolRef::Type SymbolTy;
    std::error_code ec;

    std::set<std::string> asm_functions;

    if (Dis->getExecutable()->symbols().begin() ==
        Dis->getExecutable()->symbols().end())
      return asm_functions;

    for (object::symbol_iterator I = Dis->getExecutable()->symbols().begin(),
                                 E = Dis->getExecutable()->symbols().end();
         I != E; ++I) {
      // Retrieve symbols information
      if ((ec = I->getType(SymbolTy))) continue;
      if ((ec = I->getAddress(SymbolAddr))) continue;
      if ((ec = I->getName(SymbolName))) continue;

      if (SymbolName.find("heap_low") != std::string::npos) printf("Get it !");

      switch (SymbolTy) {
        case object::SymbolRef::ST_Unknown:
          importUnknown(SymbolName, SymbolAddr);
          break;
        case object::SymbolRef::ST_Data:
          importData(SymbolName, SymbolAddr);
          break;
        case object::SymbolRef::ST_Debug:
          importDebug(SymbolName, SymbolAddr);
          break;
        case object::SymbolRef::ST_File:
          importFile(SymbolName, SymbolAddr);
          break;
        case object::SymbolRef::ST_Function: {
          // std::string name = importFunction(SymbolName, SymbolAddr);
          // if(name != "") {
          //   asm_functions.insert(name);
          // }
          break;
        }
        case object::SymbolRef::ST_Other:
          importOther(SymbolName, SymbolAddr);
          break;
      }
    }
    return asm_functions;
  }

 private:
  static void importUnknown(StringRef name, uint64_t address) {
    //XXX: Disable external symbols replacement errors occurs for some samples
    // It seems that this function is not able to replace all references...
    return;

    GlobalVariable* var = IContext::Mod->getGlobalVariable(name);

    if (var == NULL) {
      // How to guess the size ?
      // inception_warning("Unused symbol %s", name.str().c_str());
    } else {
      if (!var->hasInitializer()) {  // Replace by an internally linked obj
        inception_warning("Replacing external symbol %s", name.str().c_str());

        std::string newName = name.str() + "_substitute";

        Type* Ty;
        if(var->getType()->isPointerTy()) {
          Ty = var->getType()->getPointerElementType();
        } else
          Ty =  var->getType();

        Value* substitute = Reg(StringRef(newName), Ty);

        // bool remove = true;

        for (auto b = var->user_begin(); b != var->user_end(); b++) {
          // if( isa<Constant>(*b) || !isa<GlobalValue>(*b)) {
          //   b->dump();
          //   remove = false;
          //   break;
          // }


          b->replaceUsesOfWith(var, substitute);

          for (auto o = b->op_begin(); o != b->op_end(); o++) {
            User* u = cast<User>(*b);
            u->replaceUsesOfWith(var, substitute);
          }
        }

        // var->replaceAllUsesWith(substitute);

        // if(remove) {
          var->dropAllReferences();
          var->eraseFromParent();
        // }
      }
    }
  }

  static void importData(StringRef name, uint64_t address) {}

  static void importDebug(StringRef name, uint64_t address) {}

  static void importFile(StringRef name, uint64_t address) {}

  static std::string importFunction(StringRef name, uint64_t address) {
    Function* Func = IContext::Mod->getFunction(name);
    if (Func == NULL) {
      FunctionType* FType = FunctionType::get(
          Type::getPrimitiveType(IContext::Mod->getContext(), Type::VoidTyID),
          false);

      Constant* fct = IContext::Mod->getOrInsertFunction(name, FType);
      Function* function = dyn_cast<Function>(fct);

      BasicBlock* bb =
          BasicBlock::Create(IContext::Mod->getContext(), "entry", function);

      IRBuilder<>* IRB = new IRBuilder<>(bb);
      IRB->CreateRetVoid();

      return function->getName().str();
    }
    return "";
  }

  static void importOther(StringRef name, uint64_t address) {
    inception_warning("What should I do with %s at 0x%8x", name.str().c_str(),
                      (uint32_t)address);
  }
};

#endif
