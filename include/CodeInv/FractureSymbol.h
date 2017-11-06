//===--- FractureSymbol - Extended Class from LLVM SymbolRef -----*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class inherits from llvm::object::SymbolRef with the intention of adding
// data to existing symbols or creating symbols in stripped binaries.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: August 28, 2013
//===----------------------------------------------------------------------===//

#ifndef FRACTURE_SYMBOL_H
#define FRACTURE_SYMBOL_H

#include "llvm/Object/Error.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormattedStream.h"

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
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"

#include <map>

using namespace llvm;
using namespace inception;

namespace fracture {

class FractureSymbol : public object::SymbolRef {
  public:
    FractureSymbol() {}
    FractureSymbol(uint64_t A,
                   StringRef N,
                   uint32_t Al,
                   SymbolRef::Type T,
                   uint64_t S) {

      Address = A;
      Name = N;
      Alignment = Al;
      Type = T;
      Size = S;
    }
    // Constructs a FractureSymbol using an existing SymbolRef
    FractureSymbol(const object::SymbolRef &f) : object::SymbolRef(f) {}

    // Method overrides
    std::error_code getAddress(uint64_t &Result) const;
    std::error_code getName(StringRef &Result) const;
    std::error_code getAlignment(uint32_t &Result) const;
    std::error_code getType(SymbolRef::Type &Result) const;
    std::error_code getSize(uint64_t &Result) const;


    // matchAddress() sets the address of a dynamic FractureSymbol by
    // pairing the symbol name with the original call instruction offset
    void matchAddress(std::map<StringRef, uint64_t> Rels);

    // Getters and setters
    void setAddress(uint64_t Addr);

    void setName(StringRef name);

    uint64_t getadd() {
      return Address;
    }
    StringRef getN() {
      return Name;
    }

    bool error(std::error_code ec) {
      if (!ec) return false;

      inception_warning("Error reading file: %s", ec.message().c_str());
      return true;
    }

    void dump() {
      StringRef Name;
      uint64_t Addr = 0;
      object::SymbolRef::Type Type;
      uint64_t Size;

      if (error(getName(Name))) return;
      if (error(getAddress(Addr))) return;
      if (error(getType(Type))) return;
      if (error(getSize(Size))) return;

      char FileFunc = ' ';
      if (Type == object::SymbolRef::ST_File)
        FileFunc = 'f';
      else if (Type == object::SymbolRef::ST_Function)
        FileFunc = 'F';

      outs() << format("%08" PRIx64, Addr) << " "
             << FileFunc  // Name of function (F), file (f) or object (O).
             << ' ';
      outs() << '\t' << format("%08" PRIx64 " ", Size) << Name << '\n';
    }

  private:
    uint64_t Address = 0;
    StringRef Name ;
    uint32_t Alignment;
    SymbolRef::Type Type;
    uint64_t Size;
};

} // end namespace fracture

#endif
