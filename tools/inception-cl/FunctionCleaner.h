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

#ifndef FUNCTION_CLEANER_H
#define FUNCTION_CLEANER_H

class FunctionCleaner {
 public:
  FunctionCleaner() {}

  ~FunctionCleaner() {}

  static void Clean(llvm::Function* fct) {
    DropInstructions(fct);

    DropBasicBlocks(fct);
  }

 private:
  static void DropBasicBlocks(llvm::Function* fct) {
    std::vector<BasicBlock*> removeList;

    for (auto bb_i = fct->begin(); bb_i != fct->end(); ++bb_i) {
      BasicBlock* bb = &*bb_i;
      removeList.push_back(bb);
    }

    std::vector<BasicBlock*>::reverse_iterator rit;
    for (rit = removeList.rbegin(); rit != removeList.rend(); ++rit) {
      (*rit)->dropAllReferences();
      (*rit)->eraseFromParent();
    }
    removeList.clear();
  }

  static void DropInstructions(llvm::Function* fct) {
    std::vector<Instruction*> removeList;

    for (auto bb_i = fct->begin(); bb_i != fct->end(); ++bb_i) {
      Instruction* inst = (*bb_i).begin();

      while (inst != (*bb_i).end() || inst == nullptr) {
        inst->replaceAllUsesWith(UndefValue::get(inst->getType()));
        bool append = true;
        for (auto element : removeList)
          if (element == inst) append = false;

        if (append) {
          removeList.push_back(inst);
        }
        inst = inst->getNextNode();
      }

      std::vector<Instruction*>::reverse_iterator rit;
      for (rit = removeList.rbegin(); rit != removeList.rend(); ++rit) {
        (*rit)->dropAllReferences();
        (*rit)->eraseFromParent();
      }
      removeList.clear();
    }
  }

  /*
   * Prepare for erase by replacing uses by null value and adding to the remove
   * list all the chained instructions.
   * Also erase MetaData.
   */
  static void PrepareForErase(llvm::Instruction* instruction,
                              std::vector<Instruction*> removeList) {
    // Instruction* next = nullptr;
    // bool linked = true;
    //
    // while (linked) {
    //   next = inst->getNextNode();
    //   if (next == (*bb_i).end() || next == nullptr) break;
    //   linked = false;
    //   for (auto U : next->users()) {
    //     if (auto I = dyn_cast<Instruction>(U)) {
    //       if (I == next) linked = true;
    //     }
    //   }
    // }

    if (instruction == nullptr) return;

    for (auto U : instruction->users()) {
      Instruction* user = cast<Instruction>(U);
      PrepareForErase(user, removeList);
    }

    instruction->replaceAllUsesWith(UndefValue::get(instruction->getType()));

    SmallVector<std::pair<unsigned, MDNode*>, 100> Metadata;

    instruction->getAllMetadata(Metadata);

    for (unsigned i = 0, n = Metadata.size(); i < n; ++i) {
      unsigned Kind = Metadata[i].first;

      instruction->setMetadata(Kind, nullptr);
    }
    Metadata.clear();

    bool append = true;
    for (auto element : removeList)
      if (element == instruction) append = false;

    if (append) {

      removeList.push_back(instruction);
    }
  }
};
#endif
