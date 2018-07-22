
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

#include "IRMerger.h"

#include "FunctionCleaner.h"
#include "Utils/ABIAdapter.h"
#include "Utils/Builder.h"
#include "Utils/IContext.h"

extern bool nameLookupAddr(StringRef funcName, uint64_t& Address);

namespace fracture {

IRMerger::IRMerger(Decompiler* P_DEC) : DEC(P_DEC) {}

IRMerger::~IRMerger() {}

void IRMerger::Run(llvm::StringRef name) {
  Module* mod = IContext::Mod;

  Function* fct = mod->getFunction(name);

  uint64_t address;
  Disassembler* DIS = (Disassembler*)DEC->getDisassembler();
  if (nameLookupAddr(name, address, DIS) == false) {
    Module* mod = IContext::Mod;
    Function* fct = mod->getFunction(name);

    FunctionCleaner::Clean(fct);
    inception_message(
        "Function %s removed because it's not defined in the symbols table",
        name.str().c_str());
    return;
  }

  FunctionCleaner::Clean(fct);

  if (fct->empty()) {
    BasicBlock::Create(mod->getContext(), "entry", fct);
    fct->addFnAttr("Empty", "True");
  }

  WriteABIPrologue(fct);

  Decompile(name);

  WriteABIEpilogue(fct);
}

void IRMerger::Decompile(llvm::StringRef name) {
  uint64_t address;

  Disassembler* DIS = (Disassembler*)DEC->getDisassembler();

  if (nameLookupAddr(name, address, DIS) == false) {
    inception_error(
        "Decompile failed because %s is not present in the symbols table",
        name.str().c_str());
  }

  std::string fileName = name.str() + std::string(".dis");

  DIS->setDisassFileNameAndAddr(fileName, address);

  std::error_code ErrorInfo;
  raw_fd_ostream FOut(fileName, ErrorInfo, sys::fs::OpenFlags::F_RW);
  formatted_raw_ostream Out(FOut, false);

  StringRef SectionName;
  object::SectionRef Section = DIS->getSectionByAddress(address);
  DIS->setSection(Section);

  if (DIS->printInstructions(Out, address, 0, false) == 0)
    inception_warning("Cannot create disassembled file for %s", name);

  DEC->decompile(address);
}

/*
 * Transform the return instruction to a return with the right type
 */
void IRMerger::WriteABIEpilogue(llvm::Function* fct) {
  ReturnInst* ret;

  Type* FType = fct->getReturnType();

  Instruction* last = &(fct->back().back());
  if (last && !dyn_cast<ReturnInst>(last)) {
    llvm::ReturnInst::Create(getGlobalContext(), NULL, &(fct->back()));
  }

  // void return has been write by Lifter
  if (FType->isVoidTy()) {
    return;
  }

  bool has_return_inst = false;
  Instruction* inst;
  for (auto bb = fct->begin(); bb != fct->end(); bb++) {
    inst = (*bb).begin();

    while (inst != (*bb).end() || inst == nullptr) {
      auto next = inst->getNextNode();

      if ((ret = dyn_cast<ReturnInst>(inst)) != NULL) {
        IRBuilder<>* IRB = new IRBuilder<>(inst);

        ABIAdapter abi;
        Value* Res = abi.Higher(FType, IRB);

        IRB->CreateRet(Res);

        inst->eraseFromParent();
        has_return_inst = true;
      }
      inst = next;
    }
  }
}

void IRMerger::WriteABIPrologue(llvm::Function* fct) {
  BasicBlock& new_entry = fct->getEntryBlock();

  IRBuilder<>* IRB = new IRBuilder<>(&new_entry);

  Module* mod = DEC->getModule();

  ABIAdapter abi;

  for (auto arg = fct->arg_begin(); arg != fct->arg_end(); arg++) {
    Value* Res = NULL;

    Res = abi.Lower(arg, IRB);

    // if (arg->getType()->isPointerTy()) {
    //   Res = IRB->CreatePtrToInt(arg, IntegerType::get(mod->getContext(),
    //   32));
    // }
    //
    // if (arg->getType()->isIntegerTy()) {
    //   Res = IRB->CreateZExt(arg, IntegerType::get(mod->getContext(), 32));
    // }
    //
    // if (arg->getType()->isArrayTy()) {
    //   for (uint64_t i = 0; i < arg->getType()->getArrayNumElements(); i++) {
    //     if (i != 0) reg_counter++;
    //     reg = Reg(StringRef("R" + std::to_string(reg_counter)));
    //     Value* element = IRB->CreateExtractValue(arg, i);
    //     Res = IRB->CreateStore(element, reg);
    //   }
    //   continue;
    // }
    // IRB->CreateStore(Res, reg);
  }
}

}  // namespace fracture
