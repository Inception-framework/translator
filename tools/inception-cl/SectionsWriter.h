#ifndef SECTION_WRITER_H
#define SECTION_WRITER_H

#include "CodeInv/Decompiler.h"
#include "CodeInv/Disassembler.h"
#include "CodeInv/FractureMemoryObject.h"
#include "llvm/ADT/IndexedMap.h"
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

#include "Utils/Builder.h"

using namespace fracture;
using namespace llvm;

class SectionsWriter {
 public:
  static void WriteSection(StringRef SectionName, const Disassembler* Dis,
                           Module* mod) {
    // Locals
    ConstantInt *c_addr, *constant, *c_4;

    if (mod->getGlobalVariable(SectionName) != NULL) return;

    // Prepare section
    StringRef Bytes;
    const object::SectionRef Section = Dis->getSectionByName(SectionName);

    if (Section.getSize() == 0) return;

    std::error_code ec = Section.getContents(Bytes);
    if (ec) {
      llvm::errs() << ec.message();
      return;
    }
    unsigned size = Section.getSize();
    FractureMemoryObject* CurSectionMemory =
        new FractureMemoryObject(Bytes, Section.getAddress());

    // Set Insertion point
    Function* fct = mod->getFunction("main");
    BasicBlock* bb = &(fct->getEntryBlock());
    Instruction* inst = &(bb->front());

    inception_message("%s ...", SectionName.str().c_str());

    // IRBuilder to insert new instruction after inst
    IRBuilder<>* IRB = new IRBuilder<>(inst);

    // Declare the data section
    Type* Ty =
        ArrayType::get(IntegerType::get(mod->getContext(), 32), size / 4);
    Constant* Initializer = Constant::getNullValue(Ty);

    GlobalVariable* DataSection = new GlobalVariable(
        /*Module=*/*mod,
        /*Type=*/Ty,
        /*isConstant=*/false,
        /*Linkage=*/GlobalValue::CommonLinkage,
        /*Initializer=*/Initializer,  // has initializer, specified below
        /*Name=*/StringRef(SectionName));
    DataSection->setAlignment(4);

    //We do not need to initialize the heap
    if (SectionName.equals(".heap") == 0) return;

    c_4 = ConstantInt::get(mod->getContext(), APInt(32, 2));
    Value* R0 = Reg("R0");

    constant = ConstantInt::get(mod->getContext(), APInt(32, 0));

    for (uint64_t i = 0; i < (size / 4); i++) {
      uint64_t Address = i * 4 + CurSectionMemory->getBase();

      if (SectionName.equals(".data")) {
        uint8_t* B = new uint8_t(4);
        int NumRead = CurSectionMemory->readBytes(B, Address, 4);
        if (NumRead < 0) {
          llvm::errs() << "Unable to read current section memory!\n";
          return;
        }

        uint32_t val = B[0] | (B[1] << 8) | (B[2] << 16) | (B[3] << 24);
        constant = ConstantInt::get(mod->getContext(), APInt(32, val));
      }

      c_addr = ConstantInt::get(mod->getContext(), APInt(32, Address));

      Value* ptr = IRB->CreateIntToPtr(c_addr, R0->getType());

      IRB->CreateStore(constant, ptr);

      ptr = IRB->CreateAdd(c_addr, c_4);
    }
  }
};
#endif
