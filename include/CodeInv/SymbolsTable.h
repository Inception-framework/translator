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

#ifndef SYMBOLS_TABLE_H
#define SYMBOLS_TABLE_H

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "CodeInv/FractureSymbol.h"
#include "Utils/ErrorHandling.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/Error.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"

using namespace inception;
using namespace llvm;

namespace fracture {

class SymbolsTable {
 public:
  SymbolsTable(object::ObjectFile *Executable) {
    /*
     * Import symbols table
     * Cast to supported ELF format
     */
    if (const object::ELF32LEObjectFile *elf =
            dyn_cast<const object::ELF32LEObjectFile>(Executable)) {
      importSymbols(elf);
    } else if (const object::ELF32BEObjectFile *elf =
                   dyn_cast<const object::ELF32BEObjectFile>(Executable)) {
      importSymbols(elf);
    } else if (const object::ELF64BEObjectFile *elf =
                   dyn_cast<const object::ELF64BEObjectFile>(Executable)) {
      importSymbols(elf);
    } else if (const object::ELF64LEObjectFile *elf =
                   dyn_cast<const object::ELF64LEObjectFile>(Executable)) {
      importSymbols(elf);
    } else
      inception_error("Unsupported ELF format");
  }

  ~SymbolsTable() {
    for (auto &it : Syms) delete it;
  }

  bool error(std::error_code ec) {
    if (!ec) return false;

    inception_warning("Error reading file: %s", ec.message().c_str());
    return true;
  }

  /*
   * This function return the size of the symbol associated with the asked
   * address
   */
  int getSymbolSize(unsigned Address) {
    uint64_t Size;

    FractureSymbol *s = getSymbolInfo(Address);
    if (s == NULL) return -1;

    if (error(s->getSize(Size))) return -1;

    return Size;
  }

  std::string getSymbolName(unsigned Address) {
    StringRef Name;

    FractureSymbol *s = getSymbolInfo(Address);
    if (s == NULL) return "";

    if (error(s->getName(Name))) return "";

    return Name.str();
  }

  const StringRef getFunctionName(unsigned Address) {
    FractureSymbol *symbol = NULL;

    for (std::vector<FractureSymbol *>::iterator si = Syms.begin(),
                                                 se = Syms.end();
         si != se; ++si) {
      FractureSymbol *current_symbol = (*si);
      if (SymbolComparator(current_symbol, Address)) {
        if (SymbolComparator(current_symbol, object::SymbolRef::ST_Function)) {
          // current_symbol->dump();
          symbol = current_symbol;
          break;
        }
      }
    }

    if (symbol == NULL) return "";

    StringRef Name;
    if (error(symbol->getName(Name))) return "";

    if (Name.empty()) {
      std::string *FName = new std::string();
      raw_string_ostream FOut(*FName);
      FOut << "func_" << format("%1" PRIx64, Address);
      return StringRef(FOut.str());
    }
    return Name;
  }

  bool isFunctionInSymbolTable(unsigned Address) {
    return getFunctionName(Address) != "";
  }

  int getSymbolAddress(StringRef Name) {
    uint64_t Address;

    FractureSymbol *s = getSymbolInfo(Name);
    if (s == NULL) return -1;

    if (error(s->getAddress(Address))) return -1;

    return Address;
  }
  /*
   * This function check if a symbol is defined at the specified address
   */
  int getSymbolType(unsigned Address) {
    object::SymbolRef::Type Type;

    FractureSymbol *s = getSymbolInfo(Address);
    if (s == NULL) return -1;

    if (error(s->getType(Type))) return -1;

    return (int)Type;
  }

  bool SymbolComparator(FractureSymbol *s, unsigned value) {
    uint64_t Addr;

    if (error(s->getAddress(Addr))) return false;
    return (unsigned)Addr == value;
  }

  bool SymbolComparator(FractureSymbol *s, StringRef value) {
    StringRef Name;

    if (error(s->getName(Name))) return false;

    if (Name == "$d" || Name == "$a" || Name == "$t") return false;

    return Name.equals(value);
  }

  bool SymbolComparator(FractureSymbol *s, object::SymbolRef::Type Type) {
    object::SymbolRef::Type Ty;

    if (error(s->getType(Ty))) return false;

    return Type == Ty;
  }

  /*
   * Return the symbol found with address = address
   */
  FractureSymbol *getSymbolInfo(unsigned address) { return getSymbol(address); }

  /*
   * Return the symbol found with name = name
   */
  FractureSymbol *getSymbolInfo(StringRef name) { return getSymbol(name); }

  FractureSymbol *getSymbol(StringRef expected) {
    for (std::vector<FractureSymbol *>::iterator si = Syms.begin(),
                                                 se = Syms.end();
         si != se; ++si) {
      if (SymbolComparator(*si, expected)) return *si;
    }
    return NULL;
  }

  FractureSymbol *getSymbol(unsigned expected) {
    for (std::vector<FractureSymbol *>::iterator si = Syms.begin(),
                                                 se = Syms.end();
         si != se; ++si) {
      if (SymbolComparator(*si, expected)) return *si;

      // if ((this->*comparator)(*si, expected)) return *si;
      // // Syms is a sorted vector, no need to continue if current address is
      // higher if(Addr > Address)
      //   break;
    }
    return NULL;
  }

  std::vector<FractureSymbol *> *getOccurences(uint64_t address) {
    std::vector<FractureSymbol *> *list = new std::vector<FractureSymbol *>();

    for (std::vector<FractureSymbol *>::iterator si = Syms.begin(),
                                                 se = Syms.end();
         si != se; ++si) {
      StringRef Name;
      if (error((*si)->getName(Name))) continue;

      if (SymbolComparator(*si, address)) {
        if (Name.find("$") == StringRef::npos &&
            Name.find("/") == StringRef::npos) {
          list->push_back(*si);
        }
      }
    }
    return list;
  }

  //===---------------------------------------------------------------------===//
  /// lookupELFName   - With an ELF file, lookup a function address based on its
  /// name.
  ///
  /// @param Executable - The executable under analysis.
  ///
  bool lookUpSymbol(StringRef name, uint64_t &Address) {
    Address = 0;

    FractureSymbol *sym = getSymbol(name);
    if (sym == NULL)
      return false;
    else {
      if (error(sym->getAddress(Address))) return false;

      if (Address == object::UnknownAddressOrSize) {
        Address = 0;
        return false;
      } else
        return true;
    }
  }

  static bool symbolSorter(FractureSymbol *symOne, FractureSymbol *symTwo) {
    uint64_t addrOne = 0, addrTwo = 0;
    symOne->getAddress(addrOne);
    symTwo->getAddress(addrTwo);
    return addrOne < addrTwo;
  }

  template <class ELFT>
  void importSymbols(const object::ELFObjectFile<ELFT> *elf) {
    std::error_code ec;

    for (object::symbol_iterator si = elf->symbols().begin(),
                                 se = elf->symbols().end();
         si != se; ++si) {
      Syms.push_back(new FractureSymbol(*si));
    }

    for (object::symbol_iterator si = elf->dynamic_symbol_begin(),
                                 se = elf->dynamic_symbol_end();
         si != se; ++si) {
      FractureSymbol *temp = new FractureSymbol(*si);
      Syms.push_back(temp);
    }

    // Sort symbols by address
    std::sort(Syms.begin(), Syms.end(), &SymbolsTable::symbolSorter);

    // for (std::vector<FractureSymbol *>::iterator si = Syms.begin(),
    //                                              se = Syms.end();
    //      si != se; ++si) {
    //   if (error(ec)) {
    //     return;
    //   }
    //   (*si)->dump();
    // }
  }

  std::vector<FractureSymbol *> Syms;
};
}  // namespace fracture

#endif
