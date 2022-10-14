//===--- Disassembler - Interface to MCDisassembler -------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class interfaces with the MCDisassembler, and holds state relevant to
// machine functions and instructions that can be tracked by the dish tool.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: August 28, 2013
//
//
// Copyright (c) 2017 Maxim Integrated, Inc.
// Author: Nassim Corteggiani <nassim.corteggiani@maximintegrated.com>
//
// Copyright (c) 2017 EURECOM, Inc.
// Author: Giovanni Camurati <giovanni.camurati@eurecom.fr>
//===----------------------------------------------------------------------===//

#include "CodeInv/Disassembler.h"
#include "Utils/ErrorHandling.h"

using namespace llvm;

namespace fracture {

Disassembler::Disassembler(MCDirector *NewMC, object::ObjectFile *NewExecutable,
                           Module *NewModule, raw_ostream &InfoOut,
                           raw_ostream &ErrOut)
    : Infos(InfoOut), Errs(ErrOut) {

  MC = NewMC;
  setExecutable(NewExecutable);
  // If the module is null then create a new one
  if (NewModule == NULL) {
    // TODO: getloadName may fail, how to resolve?
    TheModule = new Module(Executable->getFileName(), *MC->getContext());
  } else {
    TheModule = NewModule;
  }
  // Set current section to ".text"
  // setSection(".text");
  setSection("code");
  // Initialize the MMI
  MMI = new MachineModuleInfo(*MC->getMCAsmInfo(), *MC->getMCRegisterInfo(),
                              MC->getMCObjectFileInfo());
  // Initialize the GCMI
  GMI = new GCModuleInfo();

  syms = new SymbolsTable(Executable);

  hasReachAnotherFunction = false;
}

Disassembler::~Disassembler() {
  // Note: BasicBlocks and Functions are also a part of TheModule, but we
  // still check to make sure they get deleted anyway.
  delete MC;
  delete TheModule;
  delete GMI;
  delete MMI;

  // NOTE: Consider using OwningPtr interface instead of this.
  for (std::map<unsigned, MCInst *>::iterator I = Instructions.begin(),
                                              E = Instructions.end();
       I != E; ++I) {
    if (I->second) delete I->second;
  }

  for (std::map<unsigned, MachineFunction *>::iterator I = Functions.begin(),
                                                       E = Functions.end();
       I != E; ++I) {
    if (I->second) {
      delete I->second;
      errs() << "Disassembler: MachineFunction not deleted in module!\n";
    }
  }

  // BasicBlock destructor is private.
  for (std::map<unsigned, MachineBasicBlock *>::iterator
           I = BasicBlocks.begin(),
           E = BasicBlocks.end();
       I != E; ++I) {
    // if (I->second) delete I->second;
    if (I->second) errs() << "Disassembler: BasicBlock not deleted!\n";
  }

  delete CurSectionMemory;
  delete Executable;
  delete syms;
}

MachineFunction *Disassembler::disassemble(unsigned Address,
                                           unsigned entryAddress, Function *F) {
  MachineFunction *MF = getOrCreateFunction(Address);

  if (MF->size() == 0) {
    // Decode basic blocks until end of function
    unsigned Size = 0;
    MachineBasicBlock *MBB;
    do {
      unsigned MBBSize = 0;
      MBB = decodeBasicBlock(Address + Size, entryAddress, MF, F, MBBSize);
      Size += MBBSize;
    } while (Address + Size < CurSectionEnd && MBB->size() > 0 &&
             hasReturnInstruction(MBB) == false);
    //      && !(MBB->instr_rbegin()->isReturn()));
    if (Address + Size < CurSectionEnd && MBB->size() > 0) {
      // FIXME: This can be shoved into the loop above to improve performance
      MachineFunction *NextMF = getNearestFunction(
          getDebugOffset(MBB->instr_rbegin()->getDebugLoc()));
      if (NextMF != NULL) {
        Functions.erase(
            getDebugOffset(NextMF->begin()->instr_begin()->getDebugLoc()));
      }
    }
  }

  Functions[Address] = MF;
  return MF;
}

bool Disassembler::hasReturnInstruction(MachineBasicBlock *MBB) {
  for (MachineBasicBlock::iterator I = MBB->instr_begin(), E = MBB->instr_end();
       I != E; ++I) {
    MachineInstr *instr = &(*I);

    const MCRegisterInfo *RI = getMCDirector()->getMCRegisterInfo();

    if (instr->isReturn()) {
      // outs() << "isReturn\n";
      return true;
    }

    if (instr->isBranch()) {
      if (instr->readsVirtualRegister(RI->getRARegister())) {
        // outs() << "is bx lr\n";
        return true;
      }
    }

    if (instr->getOpcode() == 2766) {  // tPOP
      if (instr->readsVirtualRegister(RI->getProgramCounter())) {
        // outs() << "is pop pc\n";
        return true;
      }
    }

    unsigned address = getDebugOffset(instr->getDebugLoc());
    unsigned size = this->getMachineInstr(address)->getDesc().Size;

    // if (I != MBB->instr_begin()) {
    object::SymbolRef::Type SymbolTy;
    unsigned next = address + size - 1;

    /*
     * This loop looks for all contiguous symbols after current instruction.
     * If any of these symbols is a Function we found the begenning of another
     * function and so the end of the current.
     * If we only find other kind of symbol we ignore them.
     */
    if (hasReachAnotherFunction) return true;
    // for (auto j = -1; j < 2; j++) {
    //   if (syms->isFunctionInSymbolTable(next + j)) {
    //     return true;
    //   }
    // }
  }
  return false;
}

bool Disassembler::hasPCReturnInstruction(MachineBasicBlock *MBB) {
  for (MachineBasicBlock::iterator I = MBB->instr_begin(), E = MBB->instr_end();
       I != E; ++I) {
    MachineInstr *instr = &(*I);

    const MCRegisterInfo *RI = getMCDirector()->getMCRegisterInfo();

    if (instr->isReturn()) {
      return true;
    }

    if (instr->getOpcode() == 2766) {  // tPOP
      if (instr->readsVirtualRegister(RI->getProgramCounter())) {
        return true;
      }
    }
  }
  return false;
}

MachineBasicBlock *Disassembler::decodeBasicBlock(unsigned Address,
                                                  unsigned entryAddress,
                                                  MachineFunction *MF,
                                                  Function *F, unsigned &Size) {
  assert(MF && "Unable to decode basic block without Machine Function!");

  // give a name to the basic block: name+offset, relative to the entry point
  uint64_t Off = Address;
  std::string MBBName;
  if (F == NULL) {
    MBBName = MF->getName().str();
    Off -= MF->getFunctionNumber();  // FIXME: Horrible, horrible hack
  } else {
    MBBName = F->getName().str();
    Off -= entryAddress;
  }
  MBBName += "+";
  MBBName += std::to_string(Off);

  // Dummy holds the name.
  BasicBlock *Dummy = BasicBlock::Create(*MC->getContext(), MBBName);
  MachineBasicBlock *MBB = MF->CreateMachineBasicBlock(Dummy);
  MF->push_back(MBB);

  // NOTE: Might also need SectAddr...
  Size = 0;

  // This function
  hasReachAnotherFunction = false;

  while (Address + Size < (unsigned)CurSectionEnd) {
    unsigned CurAddr = Address + Size;
    Size += std::max(unsigned(1), decodeInstruction(CurAddr, MBB));
    MachineInstr *MI = NULL;
    if (MBB->size() != 0) {
      MI = &(*MBB->instr_rbegin());
      MachineInstructions[CurAddr] = MI;
    }
    if (MI != NULL && MI->isTerminator()) {
      break;
    }

    if (hasReachAnotherFunction) break;

    const MCRegisterInfo *RI = getMCDirector()->getMCRegisterInfo();
    if (MI != NULL && MI->getOpcode() == 2766) {  // tPOP
      if (MI->readsVirtualRegister(RI->getProgramCounter())) {
        break;
      }
    }
  }

  if (Address >= CurSectionEnd) {
    printInfo("Reached end of current section!");
  }

  return MBB;
}

unsigned Disassembler::decodeInstruction(unsigned Address,
                                         MachineBasicBlock *Block) {
  // Disassemble instruction
  const MCDisassembler *DA = MC->getMCDisassembler();
  uint64_t InstSize;
  MCInst *Inst = new MCInst();
  ArrayRef<uint8_t> Bytes((uint8_t *)CurSectionMemory->getBytes().data(),
                          (size_t)CurSectionMemory->getBytes().size());
  // Chop any bytes off before instuction address that we don't need.
  uint64_t NewAddr = Address - CurSectionMemory->getBase();
  ArrayRef<uint8_t> NewBytes((uint8_t *)(Bytes.data() + NewAddr),
                             // Bytes.data() + Bytes.size() - NewAddr);
                             (size_t)(Bytes.size() - NewAddr));
  // Replace nulls() with outs() for stack tracing
  if (!(DA->getInstruction(*Inst, InstSize, NewBytes, Address, nulls(),
                           nulls()))) {
    printError("Unknown instruction encountered, instruction decode failed! ");

    return 1;
    // Instructions[Address] = NULL;
    // Block->push_back(NULL);
    // TODO: Replace with default size for each target.
    // return 1;
    // outs() << format("%8" PRIx64 ":\t", SectAddr + Index);
    // Dism->rawBytesToString(StringRef(Bytes.data() + Index, Size));
    // outs() << "   unkn\n";
  }
  Instructions[Address] = Inst;

  // Recover Instruction information
  const MCInstrInfo *MII = MC->getMCInstrInfo();
  MCInstrDesc *MCID = new MCInstrDesc(MII->get(Inst->getOpcode()));
  MCID->Size = InstSize;

  // RETURN INSTRUCTION HANDLING
  // In ARM, we can have several instructions acting as function returns:
  //
  // a. bx lr
  // b. pop {pc}
  // c. pop {rn1,rn2,...,pc}, ldr pc, [...]
  //
  // Follows a brief description of how code distributed around Disassembler and
  // Decompiler handles this thing:
  //
  // 1. Detection:
  //    a. detected by looking at the properties of the
  //       MachineInstruction ( it's a branch, it reads lr )
  //       done in: Disassembler::hasReturnInstruction
  //    b. similar ( it's a tPOP, it reads pc as operand )
  //       done in: Disassembler::hasReturnInstruction
  //    c. detected by looking at the MCInstrDesc properties
  //       (mayLoad and mayAffectControlFlow)
  //       done here before recovering the MachineInstruction and used
  //        by Disassembler::hasReturnInstruction
  //
  // 2. Stopping decompilation of basic blocks
  //    In Disassembler::disassemble we call the function decodeBasicBlock until
  //    the function Disassembler::hasReturnInstruction returns true
  //
  // 3. Stopping decompilation of the basic block
  //    In Disassembler::decodeBasicBlock we call the decodeInstruction function
  //    until we find a Terminator instruction (branch, including a., or c.) or
  //    c. (detected as is Disassembler::hasReturnInstruction)
  //
  // 4. Properly generating a llvm ret instruction during decompilation
  //    a. In Decompiler::createDAGFromMachineBasicBlock we detect a tBX
  //       instruction which uses LR and we transform it into a tBX_RET.
  //       The BranchLifter handler for tBX_RET will add the corresponging
  //       llvm ret instruction
  //       instruction.
  //    b. c. The Disassembler::hasPCReturnInstruction function is similar to
  //          the Disassembler::hasReturnInstruction but detects only b. and c.
  //          It is used by Decompiler::decompileBasicBlocks to inject an llvm
  //          ret instruction at the end of the last basic block if it end with
  //          b. or c. This way the normal lifter handlers for b. and c. put the
  //          normal llvm decompilation for them, but we also add the return at
  //          the end.
  //
  // 5. Here we do not discuss return parameters
  if (MCID->mayLoad() &&
      MCID->mayAffectControlFlow(*Inst, *MC->getMCRegisterInfo())) {
    MCID->Flags |= (1 << MCID::Return);
    MCID->Flags |= (1 << MCID::Terminator);
  }

  /*
   * Stop decompiling basic block if next instruction is associated with a
   * function symbol.
   */
  object::SymbolRef::Type SymbolTy;
  unsigned next_address = Address + InstSize;
  /*
   * This loop looks for all contiguous symbols after current instruction.
   * If any of these symbols is a Function we found the begenning of another
   * function and so the end of the current.
   * If we only find other kind of symbol we ignore them.
   */
  // while (is_data_block) {
  // Check if a function is defined in -1, +0, +1
  for (int k = 0; k < 2; k++)
    if (syms->isFunctionInSymbolTable(next_address + k)) {
      hasReachAnotherFunction = true;
      break;
    } else {
      // Next instruction is not a function.
      // Here we try to eliminate data
      do {
        SymbolTy =
            (object::SymbolRef::Type)syms->getSymbolType(next_address + k);
        if (SymbolTy != -1) {
          if (SymbolTy == object::SymbolRef::ST_Function) {
            hasReachAnotherFunction = true;
            break;
          } else {
            next_address += 4;
            // InstSize += 4;
          }
        }
      } while (SymbolTy != -1);
    }

  // Recover MachineInstr representation
  DebugLoc *Location = setDebugLoc(Address);
  MachineInstrBuilder MIB = BuildMI(Block, *Location, *MCID);
  unsigned int numDefs = MCID->getNumDefs();
  for (unsigned int i = 0; i < Inst->getNumOperands(); i++) {
    MCOperand MCO = Inst->getOperand(i);
    // FIXME: This hack is a workaround for the assert in
    // MachineInstr.cpp:653, where OpNo >= MCID->getNumOperands()...
    if (i >= MCID->getNumOperands() && !(MCID->isVariadic())) break;

    if (MCO.isReg()) {
      unsigned flags = 0;
      // Defs always start at the beginning of the operands list,
      // unfortunately BuildMI doesn't set default define flags so we have
      // to do it manually here.
      // NOTE: This should always be true, but might not be if operands list
      //       is not populated correctly by the MC Backend for the target.
      if (i < numDefs) {
        flags |= RegState::Define;
      }

      // NOTE: No need to worry about imp defs and uses, as these are
      // already
      //       specificed in the MCID attached to the MachineInst object.
      MIB.addReg(MCO.getReg(), flags);
      continue;
    }
    if (MCO.isImm()) {
      MIB.addImm(MCO.getImm());
      continue;
    }
    // else if (MCO.isFPImm()) MIB.addFPImm(MCO.getFPImm());
    if (MCO.isExpr()) {
      MCOperandInfo MCOpInfo = MCID->OpInfo[i];
      switch (MCOpInfo.OperandType) {
        case MCOI::OPERAND_MEMORY:
        case MCOI::OPERAND_PCREL:
        case MCOI::OPERAND_UNKNOWN:
        default:
          printError("Unknown how to handle this Expression at this time.");
      }
    }
    printError("Unknown how to handle Operand!");
  }

  // NOTE: I tried MCOpInfo here, and it appearst o be NULL
  // ... at least for ARM.
  unsigned flags = 0;
  if (MCID->mayLoad()) flags |= MachineMemOperand::MOLoad;
  if (MCID->mayStore()) flags |= MachineMemOperand::MOStore;
  if (flags != 0) {
    // Constant* cInt = ConstantInt::get(Type::getInt64Ty(ctx),
    // MCO.getImm()); Value *Val = ConstantExpr::getIntToPtr(cInt,
    // PointerType::getUnqual(Type::getInt32Ty(ctx)));
    // FIXME: note size of 4 is known to be bad for
    // some targets

    // Copy & paste set getImm to zero
    MachineMemOperand *MMO = new MachineMemOperand(MachinePointerInfo(), flags,
                                                   4, 0);  // MCO.getImm()
    MIB.addMemOperand(MMO);
    // outs() << "Name: " << MII->getName(Inst->getOpcode()) << " Flags: "
    // << flags << "\n";
  }

  // Note: I don't know why they decided instruction size needed to be 64
  // bits, but the following conversion shouldn't be an issue.
  return ((unsigned)InstSize);
}

DebugLoc *Disassembler::setDebugLoc(uint64_t Address) {
  std::string file = getDisassFileName();
  uint64_t func_addr = getDisassAddr();
  unsigned found = file.find_last_of("/");
  std::string path = file.substr(0, found);
  std::string name = file.substr(found + 1);
  SmallVector<Metadata *, 2> file_loc;
  file_loc.push_back(MDString::get(*MC->getContext(), StringRef(name)));
  file_loc.push_back(MDString::get(*MC->getContext(), StringRef(path)));

  // Note: Location stores offset of instruction, which is really a perverse
  //       misuse of this field.
  Type *Int64 = Type::getInt64Ty(*MC->getContext());
  // The following sets the "scope" variable which actually holds the address.
  uint64_t AddrMask = dwarf::DW_TAG_subprogram;
  Twine DIType = "0x" + Twine::utohexstr(AddrMask);
  std::vector<Metadata *> *Elts = new std::vector<Metadata *>();
  Elts->push_back(MDString::get(*MC->getContext(), StringRef(DIType.str())));
  Elts->push_back(MDTuple::get(*MC->getContext(), file_loc));
  Elts->push_back(ValueAsMetadata::get(ConstantInt::get(Int64, Address)));
  DIScope *Scope = new DIScope(MDNode::get(*MC->getContext(), *Elts));

  // The following is here to fill in the value and not to be used to get
  // offsets
  unsigned ColVal = (Address & 0xFF000000) >> 24;
  unsigned LineVal = ((Address - func_addr) + 1 + 1) & 0xFFFFFF;
  DebugLoc *Location =
      new DebugLoc( DILocation(LineVal, ColVal, Scope->getScope(), NULL));

  return Location;
}

MachineFunction *Disassembler::getOrCreateFunction(unsigned Address) {
  StringRef FNameRef = getFunctionName(Address);
  // MachineFunction *MF = getNearestFunction(Address); //see LIMITATIONS of
  // this function
  MachineFunction *MF = getNearestFunction(FNameRef);
  if (MF == NULL) {
    // StringRef FNameRef = getFunctionName(Address);
    // Note: this fixes breakage in the constructor below DO NOT REMOVE
    std::string FN = FNameRef.str();
    FunctionType *FTy = FunctionType::get(
        Type::getPrimitiveType(TheModule->getContext(), Type::VoidTyID), false);

    FunctionCallee inserted_callee = TheModule->getOrInsertFunction(FN, FTy);
    Function* inserted_function = dyn_cast<Function>(inserted_callee.getCallee());

    llvm::TargetMachine* TM = MC->getTargetMachine();
    const TargetSubtargetInfo &STI = *TM->getSubtargetImpl(*inserted_function);

    LLVMTargetMachine &LLVMTM = static_cast<LLVMTargetMachine &>(*TM);

    MF = new MachineFunction(*inserted_function, LLVMTM, STI, Address, *MMI);
    Functions[Address] = MF;
  }
  return MF;
}

/*
 * Return the nearest Machine Function based on the address:
 *   function entry <= Address <= function end
 *
 * LIMITATION:
 *   This function does not work well if we have two functions sharing a
 * portion of code, or in other words a function with two entry points. E.g.
 * func1: ...
 *                ...
 *         func2: ...
 *                ...
 *                ret
 *   If func1 is disassembled first, getNearestFunction(func2_entry_address)
 *   returns func1
 */
MachineFunction *Disassembler::getNearestFunction(unsigned Address) {
  if (Functions.size() == 0) {
    return NULL;
  }
  std::map<unsigned, MachineFunction *>::reverse_iterator FuncItr =
      Functions.rbegin();
  while (FuncItr != Functions.rend()) {
    if (FuncItr->second == NULL || FuncItr->second->size() == 0 ||
        FuncItr->second->rbegin()->size() == 0) {
      FuncItr++;
      continue;
    }
    if (FuncItr->first <= Address && Address < CurSectionEnd) {
      // Does this address fit there?
      MachineInstr *LastInstr =
          &(*((FuncItr->second->rbegin())->instr_rbegin()));
      if (Address <= getDebugOffset(LastInstr->getDebugLoc())) {
        return FuncItr->second;
      }
    }
    FuncItr++;
  }
  return NULL;
}

/*
 * This function is introduced to overcome the limitation of the previous one.
 * It is based on the function name instead of the function address, in order
 * to avoid ambiguity when there are multiple entry points (multiple functions
 * sharing some code in the last part).
 * The fucntion name can be obtained by calling getFunctionName(entry_address)
 * which returns the name in the symbol table (if the function is there) or an
 * a generated name (otherwise).
 */
MachineFunction *Disassembler::getNearestFunction(StringRef &NameRef) {
  if (Functions.size() == 0) {
    return NULL;
  }
  std::map<unsigned, MachineFunction *>::reverse_iterator FuncItr =
      Functions.rbegin();
  while (FuncItr != Functions.rend()) {
    if (FuncItr->second == NULL || FuncItr->second->size() == 0 ||
        FuncItr->second->rbegin()->size() == 0) {
      FuncItr++;
      continue;
    }
    if (FuncItr->second->getName().equals(NameRef)) {
      return FuncItr->second;
    }
    FuncItr++;
  }
  return NULL;
}

unsigned Disassembler::printInstructions(formatted_raw_ostream &Out,
                                         unsigned Address, unsigned Size,
                                         bool PrintTypes) {
  inception::inception_warning(
      "[printInstructions] printing only from entry to first return");
  MachineFunction *MF = disassemble(Address);

  MachineFunction::iterator BI = MF->begin(), BE = MF->end();
  // Skip to first basic block with instruction in desired address
  // Out << BI->instr_rbegin()->getDebugLoc().getLine() << "\n";
  while (BI != BE &&
         getDebugOffset(BI->instr_rbegin()->getDebugLoc()) < Address) {
    ++BI;
  }
  if (BI == BE) {
    printError(
        "Could not disassemble, reached end of function's basic blocks"
        " when looking for first instruction.");
    return 0;
  }

  MachineBasicBlock::iterator II = BI->instr_begin(), IE = BI->instr_end();
  // skip to first instruction
  while (getDebugOffset(II->getDebugLoc()) < Address) {
    if (II == IE) {
      printError(
          "Unreachable: reached end of basic block whe looking for first"
          " instruction.");
      ++BI;
      II = BI->instr_begin();
      IE = BI->instr_end();
    }
    ++II;
  }
  if (Address != getDebugOffset(II->getDebugLoc())) {
    Out << "Warning: starting at " << getDebugOffset(II->getDebugLoc())
        << " instead of " << Address << ".\n";
  }

  // Function Name and Offset
  Out << "<" << MF->getName();
  if (getDebugOffset(MF->begin()->instr_begin()->getDebugLoc()) != Address) {
    Out << "+"
        << (Address -
            getDebugOffset(MF->begin()->instr_begin()->getDebugLoc()));
  }
  Out << ">:\n";

  // Print each instruction
  unsigned InstrCount = 0;
  while (BI != BE && (Size == 0 || InstrCount < Size)) {
    printInstruction(Out, &*II, PrintTypes);
    ++InstrCount;
    ++II;
    if (II == IE) {
      ++BI;
      II = BI->instr_begin();
      IE = BI->instr_end();
    }
  }

  return InstrCount;
}

void Disassembler::printInstruction(formatted_raw_ostream &Out,
                                    MachineInstr *Inst, bool PrintTypes) {
  unsigned Address = getDebugOffset(Inst->getDebugLoc());
  unsigned Size = Inst->getDesc().getSize();
  // TODO: replace the Bytes with something memory safe (StringRef??)
  uint8_t *Bytes = new uint8_t(Size);
  int NumRead = CurSectionMemory->readBytes(Bytes, Address, Size);
  if (NumRead < 0) {
    printError("Unable to read current section memory!");
    return;
  }
  // Print Address
  Out << format("%08" PRIX64 ":", Address);
  Out.PadToColumn(12);  // 12345678: <- 9 chars + 1 space

  // Print Instruction Bytes
  for (unsigned i = 0, e = ((Size > 8) ? 8 : Size); i != e; ++i)
    Out << format("%02" PRIX8 " ", Bytes[i]);
  Out.PadToColumn(40);  // 8 bytes (2 char) + 1 space each + 2 spaces

  // Calculate function address for printing function names in disassembly
  int64_t Tgt = 0, DestInt = 0;
  StringRef FuncName;
  if (Inst->isCall() && Inst->getOpcode() != 2788 /*tSVC*/ &&
      Inst->getOpcode() != 2716 /*tBLX*/) {
    Size != 5 ? Size = 4 : Size;  // Instruction size is 4 for ARM
    for (MachineInstr::mop_iterator MII = Inst->operands_begin();
         MII != Inst->operands_end(); ++MII)
      if (MII->isImm()) DestInt = MII->getImm();
    Tgt = Address + Size + DestInt;
    FuncName = getFunctionName(Tgt);
    if (FuncName.startswith("func")) {
      StringRef SectionName;
      object::SectionRef Section = getSectionByAddress(Tgt);
      setSection(Section);
      getRelocFunctionName(Tgt, FuncName);
      Section = getSectionByAddress(Address);
      setSection(Section);
    }
  }

  // Print instruction
  // NOTE: We could print the "Full" machine instruction version here instead
  // of down-converting to MCInst...
  if (PrintTypes) {
    Inst->print(Out, MC->getTargetMachine(), false);
  } else {
    MC->getMCInstPrinter()->printInst(Instructions[Address], Address, 
                                      Inst->isCall() ? FuncName : "", *(MC->STI), Out);
    Out << "\n";
  }
  // Print the rest of the instruction bytes
  unsigned ColCnt = 8;
  for (unsigned i = 8, e = Size; i < e; ++i) {
    if (ColCnt == 8) {
      Out.PadToColumn(12);  // 8 bytes (2 char) + 1 space each + 2 spaces
      Out << "\n";
      ColCnt = 0;
    } else {
      ++ColCnt;
    }
    Out << format("%02" PRIX8 " ", Bytes[i]);
  }
  delete Bytes;

  // Print empty lines to align the next one
  for (unsigned i = 0, e = Size - 1; i != e; ++i) Out << "\n";
}

void Disassembler::setExecutable(object::ObjectFile *NewExecutable) {
  // NOTE: We don't do any reorging with the module or the machine functions.
  // We need to evaluate if this is necessary. We should *not* change the MC
  // API settings to match those of the executable.
  Executable = NewExecutable;
}

// getRelocFunctionName() pairs function call addresses with dynamically
// relocated library function addresses and sets the function name to the
// actual name rather than the function address
void Disassembler::getRelocFunctionName(unsigned Address, StringRef &NameRef) {
  MachineFunction *MF = disassemble(Address);
  MachineBasicBlock *MBB = &(MF->front());
  uint64_t JumpAddr = 0;
  StringRef real_name;
  std::error_code ec;
  bool isOffsetJump = false;

  // Iterate through the operands, checking for immediates and grabbing them
  MachineInstr *JumpInst = &*MBB->instr_rbegin();
  for (MachineInstr::mop_iterator MII = JumpInst->operands_begin();
       MII != JumpInst->operands_end(); ++MII) {
    if (MII->isImm()) {
      if (MBB->size() > 1) {
        JumpAddr = MII->getImm();
        break;
      }
      JumpAddr = MII->getImm();
    }
  }
  // If the Jump address of the instruction is smaller than the instruction
  // address then it must be an offset from the instruction address. In this
  // case, we add the jump address to the original address plus the
  // instruction size.
  if (JumpAddr < Address) isOffsetJump = true;
  if (MBB->size() > 1)
    JumpAddr += (Address + 32768 + 8);
  else if (isOffsetJump)
    JumpAddr += Address + JumpInst->getDesc().getSize();

  // Check if address matches relocation symbol address and if so
  // grab the symbol name
  for (object::section_iterator seci = Executable->section_begin();
       seci != Executable->section_end(); ++seci)
    for (object::relocation_iterator ri = seci->relocation_begin();
         ri != seci->relocation_end(); ++ri) {
      
      uint64_t relocation_address = ri->getOffset();
      relocation_address += seci->getAddress();

      if (JumpAddr == relocation_address) {
        Expected<StringRef> symbol_name_error = ri->getSymbol()->getName();

        if (!symbol_name_error) {
          //TODO: better error handling
          continue;
        }
        real_name = symbol_name_error.get();

        RelocOrigins[real_name] = Address;
      }
    }
  // NameRef is passed by reference, so if relocation doesn't match,
  // we don't want to modify the StringRef
  if (!real_name.empty()) NameRef = real_name;
}

void Disassembler::setSection(std::string SectionName) {
  setSection(getSectionByName(SectionName));
}

void Disassembler::setSection(const object::SectionRef Section) {
  StringRef section_content;
  uint64_t section_address, section_size;
  StringRef section_name;

  Expected< StringRef > section_content_error = Section.getContents();
  if (!section_content_error) {
    //TODO: Better error handling
    return;
  }
  section_content = section_content_error.get();

  section_address = Section.getAddress();
  section_size = Section.getSize();

  CurSection = Section;
  CurSectionEnd = section_address + section_size;
  CurSectionMemory = new FractureMemoryObject(section_content, section_address);

  Expected<StringRef> section_name_error = CurSection.getName();
  if(!section_name_error) {
    //TODO: Better error handling
    return;
  }
  section_name = section_name_error.get();
  printInfo("Setting Section " + std::string(section_name.data()));
}

std::string Disassembler::rawBytesToString(StringRef Bytes) {
  static const char hex_rep[] = "0123456789abcdef";

  std::string Str;

  for (StringRef::iterator i = Bytes.begin(), e = Bytes.end(); i != e; ++i) {
    Str += hex_rep[(*i & 0xF0) >> 4];
    Str += hex_rep[*i & 0xF];
    Str += ' ';
  }

  return Str;
}

const object::SectionRef Disassembler::getSectionByName(
    StringRef section_name) const {

  StringRef name;
  for (object::section_iterator si = Executable->section_begin(),
                                se = Executable->section_end();
       si != se; ++si) {
  
    Expected<StringRef> section_name_error = si->getName();
    if (! section_name_error ) {
      Expected<uint64_t> section_address_error = si->getAddress();
      uint64_t address = section_address_error.get();
      Infos << "Disassembler: Unnamed section encountered at "
            << format("%8" PRIx64, address) << "\n";
      continue;
    }
    name = *section_name_error;

    if (name.str().find(section_name.str()) != std::string::npos) return *si;
  }

  return *Executable->section_end();
}

const object::SectionRef Disassembler::getSectionByAddress(
    unsigned Address) const {
  std::error_code ec;
  for (object::section_iterator si = Executable->section_begin(),
                                se = Executable->section_end();
       si != se; ++si) {
    if (ec) {
      printError(ec.message());
      break;
    }

    uint64_t SectionAddr;
    SectionAddr = si->getAddress();

    uint64_t SectionSize;
    SectionSize = si->getSize();

    if (SectionAddr <= Address && Address < SectionAddr + SectionSize) {
      return *si;
    }
  }

  return *Executable->section_end();
}

uint64_t Disassembler::getDebugOffset(const DebugLoc &debug_location) const {
  

  MDNode *scope = debug_location.getScope();
  
  if (scope == NULL || scope->getNumOperands() != 3) {
    
    inception::inception_error(
        "[Disassembler::getDebugOffset] Scope not set properly in the "
        "debugLoc");
    return 0;
  }

  if (ConstantInt *offset = dyn_cast<ConstantInt>(
          dyn_cast<ValueAsMetadata>(scope->getOperand(2))->getValue())) {
    return offset->getZExtValue();
  }

  errs() << "Could not decode DebugOffset Value as a ConstantInt!\n";
  return 0;
}

void Disassembler::deleteFunction(MachineFunction *MF) {
  std::map<unsigned, MachineFunction *>::reverse_iterator FuncItr =
      Functions.rbegin();
  while (FuncItr != Functions.rend()) {
    if (FuncItr->second == MF) {
      break;
    }
    FuncItr++;
  }
  if (FuncItr != Functions.rend()) {
    Functions.erase(FuncItr->first);
    delete MF;
  }
}

/*
 * @Brief: Return the function name at a given address by reading the symbol table
 * @param: The unsigned int address
 */
const StringRef Disassembler::getFunctionName(uint64_t address) const {

  llvm::StringRef symbol_name;
  std::string *function_name;

  // Check in the regular symbol table first
  for (object::symbol_iterator I = Executable->symbols().begin(),
                               E = Executable->symbols().end();
       I != E; ++I) {

    // Declare expected variables
    llvm::Expected<llvm::object::SymbolRef::Type> symbol_type_error = I->getType();
    llvm::Expected<uint64_t> symbol_address_error = I->getAddress();
    llvm::Expected<llvm::StringRef> symbol_name_error = I->getName();

    // Declare symbols variables
    uint64_t symbol_address;
    llvm::object::SymbolRef::Type symbol_type;

    // Retrieve symbol type
    if(Error error = symbol_type_error.takeError()) {
      errs() << symbol_type_error.takeError() << "\n";
      continue;
    }

    // Discard non function type
    symbol_type = symbol_type_error.get();
    if (symbol_type != llvm::object::SymbolRef::ST_Function) {
      continue;
    }

    // Retrieve function address
    if(!symbol_address_error) {
      errs() << symbol_address_error.takeError() << "\n";
      continue;
    }
    symbol_address = symbol_address_error.get();

    // If address match
    if (symbol_address == address) {

      // Retrieve symbol name
      if (!symbol_name_error) {
        errs() << symbol_name_error.takeError() << "\n";
        continue;
      }
      symbol_name = *symbol_name_error;
      break;
    }
  }

  // if search was not successful
  if (symbol_name.empty()) {

    function_name = new std::string();
    raw_string_ostream ostream(*function_name);
    ostream << "func_" << format("%1" PRIx64, address);
    return StringRef(ostream.str());
  }
  return symbol_name;
}

}  // end namespace fracture
