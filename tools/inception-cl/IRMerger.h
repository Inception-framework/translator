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

//===--- IRMerger - Merge two IR Modules ----------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class uses SDNodes and emits IR. It is intended to be extended by Target
// implementations who have special ISD legalization nodes.
//
// Author: Corteggiani Nassim <nassim.corteggiani@maximintegrated.com>
// Date: April 19, 2017
//===----------------------------------------------------------------------===//

#ifndef IRMERGER_H
#define IRMERGER_H

#include "CodeInv/Decompiler.h"
#include "CodeInv/Disassembler.h"
#include "CodeInv/FractureSymbol.h"

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <algorithm>
#include "llvm/IR/Metadata.h"

#include "llvm/Object/Binary.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/Error.h"
#include "llvm/Object/ObjectFile.h"

#include "Utils/ErrorHandling.h"

#include <string>
#include <system_error>
#include <vector>

using std::string;

using namespace llvm;
using namespace fracture;
using namespace inception;

namespace fracture {

class Decompiler;
class Disassembler;
class FractureSymbol;

class IRMerger {
 public:
  IRMerger(Decompiler* DEC);

  ~IRMerger();

  void Run(llvm::StringRef name);

 protected:
  void Decompile(llvm::StringRef name);

  StringRef getIndexedValueName(StringRef BaseName);

  StringRef getBaseValueName(StringRef BaseName);

  void WriteABIPrologue(llvm::Function* fct);

  void WriteABIEpilogue(llvm::Function* fct);

  Decompiler* DEC;

  StringRef* function_name;

  ///===---------------------------------------------------------------------===//
  /// nameLookupAddr - lookup a function address based on its name.
  /// @note: COFF support function has not been written yet...
  ///
  /// @param Executable - The executable under analysis.
  ///
  bool nameLookupAddr(StringRef funcName, uint64_t& Address,
                      Disassembler* DAS) {
    bool retVal = false;
    const object::ObjectFile* Executable = DAS->getExecutable();

    // Binary is not stripped, return address based on symbol name
    if (  // const object::COFFObjectFile *coff =
        dyn_cast<const object::COFFObjectFile>(Executable)) {
      // dumpCOFFSymbols(coff, Address);
      errs() << "COFF is Unsupported section type.\n";
    } else if (const object::ELF32LEObjectFile* elf =
                   dyn_cast<const object::ELF32LEObjectFile>(Executable)) {
      retVal = lookupELFName(elf, funcName, Address, DAS);
    } else if (const object::ELF32BEObjectFile* elf =
                   dyn_cast<const object::ELF32BEObjectFile>(Executable)) {
      retVal = lookupELFName(elf, funcName, Address, DAS);
    } else if (const object::ELF64BEObjectFile* elf =
                   dyn_cast<const object::ELF64BEObjectFile>(Executable)) {
      retVal = lookupELFName(elf, funcName, Address, DAS);
    } else if (const object::ELF64LEObjectFile* elf =
                   dyn_cast<const object::ELF64LEObjectFile>(Executable)) {
      retVal = lookupELFName(elf, funcName, Address, DAS);
    } else {
      errs() << "Unsupported section type.\n";
    }
    return retVal;
  }

  //===---------------------------------------------------------------------===//
  /// lookupELFName   - With an ELF file, lookup a function address based on its
  /// name.
  ///
  /// @param Executable - The executable under analysis.
  ///
  template <class ELFT>
  static bool lookupELFName(const object::ELFObjectFile<ELFT>* elf,
                            StringRef funcName, uint64_t& Address,
                            Disassembler* DAS) {
    bool retVal = false;
    std::error_code ec;
    std::vector<FractureSymbol*> Syms;

    Address = 0;
    for (object::symbol_iterator si = elf->symbols().begin(),
                                 se = elf->symbols().end();
         si != se; ++si) {
      Syms.push_back(new FractureSymbol(*si));
    }
    for (object::symbol_iterator si = elf->dynamic_symbol_begin(),
                                 se = elf->dynamic_symbol_end();
         si != se; ++si) {
      FractureSymbol* temp = new FractureSymbol(*si);
      Syms.push_back(temp);
    }
    // if (isStripped)
    //   for (auto& it : SDAS->getStrippedGraph()->getHeadNodes()) {
    //     StringRef name = (SDAS->getMain() == it->Address
    //                           ? "main"
    //                           : DAS->getFunctionName(it->Address));
    //     FractureSymbol tempSym(it->Address, name, 0,
    //                            object::SymbolRef::Type::ST_Function, 0);
    //     Syms.push_back(new FractureSymbol(tempSym));
    //   }

    for (std::vector<FractureSymbol*>::iterator si = Syms.begin(),
                                                se = Syms.end();
         si != se; ++si) {
      if (error(ec)) {
        for (auto& it : Syms) delete it;
        return retVal;
      }

      StringRef Name;

      if (error((*si)->getName(Name))) continue;
      if (error((*si)->getAddress(Address))) continue;

      if (Address == object::UnknownAddressOrSize) {
        retVal = false;
        Address = 0;
      }

      if (funcName.str() == Name.str()) {
        retVal = true;
        for (auto& it : Syms) delete it;
        return retVal;
      }
    }
    for (auto& it : Syms) delete it;
    return retVal;
  }

  static bool error(std::error_code ec) {
    if (!ec) return false;
    inception_error("Unable to read file... %s", ec.message().c_str());
    return true;
  }
};

}  // end namespace fracture

#endif /* IRMERGER_H */
