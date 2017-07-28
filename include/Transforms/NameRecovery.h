//===--- TypeRecovery - recovers function parameters and locals -*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This function pass looks at LLVM IR from the Decompiler and tries to
// recover the function parameter types and return type of the function.
//
// Works only at the function level. It may be necessary to make this global
// across the entire decompiled program output.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: January 15, 2014
//===----------------------------------------------------------------------===//

#ifndef NAMERECOVERY_H
#define NAMERECOVERY_H

namespace llvm {

class FunctionPass;
class Pass;

FunctionPass* createNameRecoveryPass();

} // End namespace llvm


#endif
