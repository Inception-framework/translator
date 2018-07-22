/*
    This file is part of Inception translator.

    Inception translator is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Inception translator.  If not, see <https://www.gnu.org/licenses/>.

    Copyright (c) 2017 Maxim Integrated, Inc.
    Author: Nassim Corteggiani <nassim.corteggiani@maximintegrated.com>

    Copyright (c) 2017 EURECOM, Inc.
    Author: Giovanni Camurati <giovanni.camurati@eurecom.fr>
*/

#ifndef COLLISION_SOLVER_H
#define COLLISION_SOLVER_H

#include "CodeInv/Disassembler.h"
#include "CodeInv/FractureMemoryObject.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

#include "Utils/Builder.h"
#include "Utils/IContext.h"

using namespace fracture;
using namespace llvm;

/*
 * Developpers may declare symbols in Linker Script and use these symbols in
 * C/C++ code to position code inside sections (memory layout). The LLVM IR will
 * not allocate these symbols and it keeps them external. unfortunately we need
 * to link them as internal because during the execution of IR, the addresses of
 * these symbols may be required.
 *
 * Unfortunately, allocating these symbols in the IR leads to Klee fail. In
 * fact, these symbols may have the same address than others IR variables
 * (collision). As a reminder, we allocate globals variables at the same
 * location defined in the symbols table.
 *
 * One solutions is to replace variables defined in the linker script by
 * constant values.
 */

class CollisionSolver {
 public:
  static bool isLinkerDeclaration(llvm::StringRef name) {
    // std::string grep_command = "grep \"" + name.str() + " =\" link.ld ";

    std::string current_name = name.str();

    std::size_t escape;
    int counter = 0, k = 0, pos = 0;
    char to_escape[1] = {'.'};

    while (k < 1) {
      do {
        escape = current_name.find(to_escape[k], pos);

        if (escape != std::string::npos) {
          current_name.insert(escape, "\\");
          counter++;
          pos = escape + 2;
        }

      } while (counter < name.count(to_escape[k]));
      counter = pos = 0;
      k++;
    }

    std::string grep_command =
        "find . -name \"*.ld\" -exec grep \"" + current_name + " =\" {} \\;";

    // printf("Command: %s\n", grep_command.c_str());

    FILE* fp = popen(grep_command.c_str(), "r");
    char buf[1024] = "\0";

    while (fgets(buf, 1024, fp)) {
      /* do something with buf */
      // printf("Result: %s\n", buf);
    }
    fclose(fp);
    // printf("Result: %s\n", buf);

    // do we find something
    if (strlen(buf) > 2)
      return true;
    else
      return false;
  }

  static void solve(llvm::Module* mod, Disassembler* Dis) {
    uint64_t SymbolAddr;
    llvm::StringRef SymbolName;
    object::SymbolRef::Type SymbolTy;
    std::error_code ec;

    std::set<std::string> asm_functions;

    if (Dis->getExecutable()->symbols().begin() ==
        Dis->getExecutable()->symbols().end())
      return;

    for (object::symbol_iterator I = Dis->getExecutable()->symbols().begin(),
                                 E = Dis->getExecutable()->symbols().end();
         I != E; ++I) {
      // Retrieve symbols information
      if ((ec = I->getType(SymbolTy))) continue;
      if ((ec = I->getAddress(SymbolAddr))) continue;
      if ((ec = I->getName(SymbolName))) continue;

      switch (SymbolTy) {
        case object::SymbolRef::ST_Data:
        case object::SymbolRef::ST_Unknown: {
          // Look up variables declared in the linker script
          if (isLinkerDeclaration(SymbolName)) {
            // Continue if no IR declaration associated
            GlobalVariable* v1 = mod->getGlobalVariable(SymbolName);
            if (!v1) continue;

            inception_warning("\n\nSolving conflict using M1 for %s",
                              SymbolName.str().c_str());

            std::vector<User*> users;

            for (auto U : v1->users()) {
              users.push_back(U);
            }

            while (!users.empty()) {
              auto U = users.back();
              users.pop_back();
              // if (!v1 || !U || (*U == v1->user_end())) continue;

              inception_message("\nDefinition found here:");
              U->dump();

              Value* new_v = NULL;

              Type* Ty = NULL;

              for (unsigned i = 0, E = U->getNumOperands(); i != E; ++i)
                if (U->getOperand(i) == v1) {
                  Ty = U->getOperand(i)->getType();
                  break;
                }

              if (Ty == NULL) Ty = U->getType();

              unsigned size;

              Instruction* root = getRoot(U);
              if (!root) {
                inception_warning("Definition has no root instruction...");
              } else {
                if (Ty->isIntegerTy()) {
                  size = Ty->getIntegerBitWidth();
                  new_v = ConstantInt::get(IContext::getContextRef(),
                                           APInt(size, SymbolAddr));

                  root->replaceUsesOfWith(v1, new_v);
                }

                if (Ty->isPointerTy()) {
                  if (root->getOpcode() == Instruction::PtrToInt) {
                    new_v = ConstantInt::get(IContext::getContextRef(),
                                             APInt(32, SymbolAddr));

                    BasicBlock::iterator ii(root);

                    Instruction::BinaryOps Op =
                        (Instruction::BinaryOps)(Instruction::Add);

                    Instruction* o;

                    o = BinaryOperator::Create(Op, new_v, getConstant(0));

                    ReplaceInstWithInst(root->getParent()->getInstList(), ii,
                                        o);
                  } else if (root->getOpcode() == Instruction::GetElementPtr) {
                    BasicBlock::iterator ii(root);

                    new_v = ConstantInt::get(IContext::getContextRef(),
                                             APInt(32, SymbolAddr));

                    Instruction* n = new IntToPtrInst(new_v, Ty);

                    ReplaceInstWithInst(root->getParent()->getInstList(), ii,
                                        n);
                  } else {
                    new_v = ConstantInt::get(IContext::getContextRef(),
                                             APInt(32, SymbolAddr));

                    Instruction* n = new IntToPtrInst(new_v, Ty, "", root);

                    root->replaceUsesOfWith(v1, n);
                  }
                }
              }
            }

            while (!v1->use_empty()) {
              auto& u = *v1->use_begin();
              u.set(getConstant(SymbolAddr));
            }

            v1->dropAllReferences();
            v1->eraseFromParent();
          } else {
            // Is another symbol defined in the SymbolTable at the same address
            // ?
            std::vector<FractureSymbol*>* list;
            list = Dis->syms->getOccurences(SymbolAddr);

            if (list->size() < 2) continue;

            while (!list->empty()) {
              FractureSymbol* symbole = list->back();
              list->pop_back();

              StringRef Name;
              if ((ec = symbole->getName(Name))) continue;

              if (isLinkerDeclaration(Name)) continue;

              // Continue if no IR declaration associated
              GlobalVariable* v1 = mod->getGlobalVariable(Name);
              if (!v1) continue;

              inception_warning("\n\nSolving conflict using M2 for %s",
                                Name.str().c_str());

              if (!v1->getType()->getPointerElementType()->isIntegerTy())
                continue;

              std::vector<User*> users;

              for (auto U : v1->users()) {
                users.push_back(U);
              }

              while (!users.empty()) {
                auto U = users.back();
                users.pop_back();

                inception_message("\nDefinition found here:");
                U->dump();

                Value* new_v = NULL;

                Type* Ty = NULL;

                for (unsigned i = 0, E = U->getNumOperands(); i != E; ++i)
                  if (U->getOperand(i) == v1) {
                    Ty = U->getOperand(i)->getType();
                    break;
                  }

                if (Ty == NULL) Ty = U->getType();

                unsigned size;

                Instruction* root = getRoot(U);
                if (!root) {
                  inception_warning("Definition has no root instruction...");
                } else {
                  if (Ty->isIntegerTy()) {
                    size = Ty->getIntegerBitWidth();
                    new_v = ConstantInt::get(IContext::getContextRef(),
                                             APInt(size, SymbolAddr));

                    root->replaceUsesOfWith(v1, new_v);
                  }

                  if (Ty->isPointerTy()) {
                    if (root->getOpcode() == Instruction::PtrToInt) {
                      new_v = ConstantInt::get(IContext::getContextRef(),
                                               APInt(32, SymbolAddr));

                      BasicBlock::iterator ii(root);

                      Instruction::BinaryOps Op =
                          (Instruction::BinaryOps)(Instruction::Add);

                      Instruction* o;

                      o = BinaryOperator::Create(Op, new_v, getConstant(0));

                      ReplaceInstWithInst(root->getParent()->getInstList(), ii,
                                          o);
                    } else if (root->getOpcode() ==
                               Instruction::GetElementPtr) {
                      BasicBlock::iterator ii(root);

                      new_v = ConstantInt::get(IContext::getContextRef(),
                                               APInt(32, SymbolAddr));

                      Instruction* n = new IntToPtrInst(new_v, Ty);

                      ReplaceInstWithInst(root->getParent()->getInstList(), ii,
                                          n);
                    } else {
                      new_v = ConstantInt::get(IContext::getContextRef(),
                                               APInt(32, SymbolAddr));

                      Instruction* n = new IntToPtrInst(new_v, Ty, "", root);

                      root->replaceUsesOfWith(v1, n);
                    }
                  }
                }
              }

              while (!v1->use_empty()) {
                auto& u = *v1->use_begin();
                u.set(getConstant(SymbolAddr));
              }

              v1->dropAllReferences();
              v1->eraseFromParent();
            }

            delete list;
          }

          break;
        }
        case object::SymbolRef::ST_Debug:
        case object::SymbolRef::ST_File:
        case object::SymbolRef::ST_Function:
        case object::SymbolRef::ST_Other:
          break;
      }
    }
  }

 private:
  static Instruction* getRoot(User* U) {
    Instruction* inst = NULL;

    inst = dyn_cast<Instruction>(U);
    if (inst) return inst;

    for (auto u : U->users()) {
      if ((inst = dyn_cast<Instruction>(u)) == NULL)
        return getRoot(u);
      else {
        inception_message("Root found !");
        inst->dump();
        return inst;
      }
    }
    return NULL;
  }

  // static bool replaceUsesOfWith(Instruction* root, Value* old_v, Value*
  // new_v) {
  //   bool is_direct_operand = false;
  //
  //   // If this is not the case we need to remove the entire
  //   // instruction ...
  //   for (unsigned i = 0, E = root->getNumOperands(); i != E; ++i)
  //     if (root->getOperand(i) == old_v) {
  //       root->setOperand(i, new_v);
  //       is_direct_operand = true;
  //     }
  //
  //   if (!is_direct_operand) {
  //     for (unsigned i = 0, E = root->getNumOperands(); i != E; ++i) {
  //       replaceUsesOfWith(root->getOperand(i), old_v, new_v);
  //     }
  //   }
  // }
};
#endif
