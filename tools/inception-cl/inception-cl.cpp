//===--- fracture-cl.cpp - Fracture Shell -----------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The fracture shell provides a command line interface to permit the user
// to interactively disassemble and output a binary executable in LLVM-IR.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/Support/COFF.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetLowering.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetRegisterInfo.h"

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
// #include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitstreamWriter.h>
#include <llvm/Bitcode/LLVMBitCodes.h>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>
// #include <llvm/Support/Streams.h>

#include <inttypes.h>
#include <signal.h>
#include <unistd.h>  //new
#include <algorithm>
#include <chrono>
#include <cstdlib>  //new
#include <map>
#include <sstream>
#include <string>
#include <thread>

// iostream is frowned upon in LLVM, but
// we are doing console I/O here.
#include <stdio.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include "BinFun.h"
#include "CodeInv/Decompiler.h"
#include "CodeInv/Disassembler.h"
#include "Commands/Commands.h"
#include "IRMerger.h"

#include "AssemblySupport.h"
#include "FunctionsHelperWriter.h"
#include "InterruptSupport.h"
#include "SectionsWriter.h"
#include "StackAllocator.h"
#include "Utils/Builder.h"
#include "Utils/ErrorHandling.h"

using namespace llvm;
using namespace fracture;
using namespace inception;
using std::string;

static void save(std::string fileName, Module* module);

//===----------------------------------------------------------------------===//
// Global Variables and Parameters
//===----------------------------------------------------------------------===//
static std::string ProgramName;
static Commands CommandParser;

std::unique_ptr<object::ObjectFile> TempExecutable;
// bool isStripped = false;

// Command Line Options
cl::opt<std::string> TripleName("triple",
                                cl::desc("Target triple to disassemble for, "
                                         "see -version for available targets"));
cl::opt<std::string> ArchName("arch",
                              cl::desc("Target arch to disassemble for, "
                                       "see -version for available targets"));
cl::opt<std::string> InputBinaryFileName(cl::Positional,
                                         cl::desc("<input file>"),
                                         cl::init("-"));
cl::opt<std::string> InputBitcodeFileName(cl::Positional,
                                          cl::desc("<input bitcode file>"),
                                          cl::init("-"));

cl::list<std::string> MAttrs("mattr", cl::CommaSeparated,
                             cl::desc("Target specific attributes"),
                             cl::value_desc("a1,+a2,-a3,..."));

static cl::opt<bool> ViewMachineDAGs(
    "view-machine-dags", cl::Hidden,
    cl::desc("Pop up a window to show dags before Inverse DAG Select."));

static cl::opt<bool> ViewIRDAGs(
    "view-ir-dags", cl::Hidden,
    cl::desc("Pop up a window to show dags after Inverse DAG Select."));

static cl::opt<bool> StrippedBinary(
    "stripped", cl::Hidden,
    cl::desc(
        "Run stripped disassembler to locate functions in stripped binary."));

static cl::opt<bool> printGraph(
    "print-graph", cl::Hidden,
    cl::desc(
        "Print graph for stripped file, must also enable stripped command"));

///===---------------------------------------------------------------------===//
/// loadBitcode     - Tries to open the bitcode file and set the ObjectFile.
///
/// @param FileName - The name of the file to open.
///
static ErrorOr<std::unique_ptr<MemoryBuffer>> loadBitcode(StringRef FileName) {
  // File should be stdin or it should exist.
  if (FileName != "-" && !sys::fs::exists(FileName)) {
    errs() << ProgramName << ": No such file or directory: '" << FileName.data()
           << "'.\n";
    return make_error_code(std::errc::not_enough_memory);
  }

  ErrorOr<std::unique_ptr<MemoryBuffer>> fileOrErr =
      MemoryBuffer::getFileOrSTDIN(FileName);
  if (std::error_code ec = fileOrErr.getError()) {
    std::cerr << "Error opening input file: " + ec.message() << std::endl;
    return make_error_code(std::errc::not_enough_memory);
  }

  return std::move(fileOrErr);
}

static std::error_code runInception(StringRef FileName) {
  Module *module = 0;
  MCDirector *MCD = 0;
  Disassembler *DAS = 0;
  Decompiler *DEC = 0;

  // File should be stdin or it should exist.
  if (FileName != "-" && !sys::fs::exists(FileName)) {
    inception_error("No such binary file or directory : %s ", FileName.data());
  }

  ErrorOr<object::OwningBinary<object::Binary>> Binary =
      object::createBinary(FileName);
  if (std::error_code err = Binary.getError()) {
    inception_error("Unknown binary file format : %s ", FileName.data());
  } else {
    if (Binary.get().getBinary()->isObject()) {
      std::pair<std::unique_ptr<object::Binary>, std::unique_ptr<MemoryBuffer>>
          res = Binary.get().takeBinary();
      ErrorOr<std::unique_ptr<object::ObjectFile>> ret =
          object::ObjectFile::createObjectFile(
              res.second.release()->getMemBufferRef());
      TempExecutable.swap(ret.get());
    }
  }

  // Initialize the Disassembler
  std::string FeaturesStr;
  if (MAttrs.size()) {
    SubtargetFeatures Features;
    for (unsigned int i = 0; i < MAttrs.size(); ++i) {
      Features.AddFeature(MAttrs[i]);
    }
    FeaturesStr = Features.getString();
  }

  Triple TT("thumbv7m-unknown-none-elf");

  TripleName = TT.str();

  ErrorOr<std::unique_ptr<MemoryBuffer>> fileOrErr =
      loadBitcode(InputBitcodeFileName.getValue());
  if (std::error_code ec = fileOrErr.getError()) {
    return ec;
  }

  MemoryBuffer *MemBuffer = fileOrErr.get().release();
  LLVMContext context;

  ErrorOr<Module *> moduleOrErr =
      parseBitcodeFile(MemBuffer->getMemBufferRef(), context);

  if (std::error_code ec = moduleOrErr.getError()) {
    std::cerr << "Error reading Module: " + ec.message() << std::endl;
    return ec;
  }

  std::unique_ptr<Module> old_module(moduleOrErr.get());

  old_module.get()->materializeAll();
  inception_message("\n");
  inception_message("Module Name: %s",
                    old_module.get()->getName().str().c_str());
  inception_message("Module Target triple: %s",
                    old_module.get()->getTargetTriple().c_str());
  inception_message("\n");

  module = old_module.release();

  MCD = new MCDirector(TripleName, "cortex-m3", FeaturesStr, TargetOptions(),
                       Reloc::DynamicNoPIC, CodeModel::Default,
                       CodeGenOpt::Default, outs(), errs());
  DAS = new Disassembler(MCD, TempExecutable.release(), NULL, outs(), outs());
  DEC = new Decompiler(DAS, module, outs(), outs());

  DEC->setViewMCDAGs(ViewMachineDAGs);
  DEC->setViewIRDAGs(ViewIRDAGs);

  if (!MCD->isValid()) {
    errs() << "Warning: Unable to initialized LLVM MC API!\n";
    return make_error_code(std::errc::not_supported);
  }

  std::set<std::string> asm_functions;
  inception_message("\n");

  inception_message("Detecting all assembly functions ...");
  for (auto iter1 = module->getFunctionList().begin();
       iter1 != module->getFunctionList().end(); iter1++) {
    Function &old_function = *iter1;

    for (auto iter2 = old_function.getBasicBlockList().begin();
         iter2 != old_function.getBasicBlockList().end(); iter2++) {
      BasicBlock &old_bb = *iter2;
      for (auto iter3 = old_bb.begin(); iter3 != old_bb.end(); iter3++) {
        const CallInst *ci = dyn_cast<CallInst>(iter3);

        if (ci != NULL)
          if (isa<InlineAsm>(ci->getCalledValue())) {
            asm_functions.insert(old_function.getName().str());
          }
      }  // END FOR INSTRUCTIOn
    }    // END FOR BB
  }      // END FOR FCT
  inception_message("Done -> %ld functions.", asm_functions.size());

  initAPI(module, DEC);

  inception_message("Importing pure assembly code...");
  AssemblySupport::ImportAll(module, DAS);
  inception_message("Done\n");

  IRMerger *merger = new IRMerger(DEC);

  for (auto &str : asm_functions) {
    inception_message("Processing function %s...", str.c_str());

    merger->Run(llvm::StringRef(str));
    inception_message("Done\n");
  }
  inception_message("Decompilation stage done\n");

  inception_message("Allocating and initializing virtual stack...");
  StackAllocator::Allocate(module, DAS);
  StackAllocator::InitSP(module, DAS);
  inception_message("Done\n");

  inception_message("Importing sections .data, .bss and .heap...");
  SectionsWriter::WriteSection(".data", DAS, module);
  SectionsWriter::WriteSection(".bss", DAS, module);
  SectionsWriter::WriteSection(".heap", DAS, module);
  SectionsWriter::WriteSection(".interrupt_vector", DAS, module);
  inception_message("Done\n");

  inception_message("Adding call to functions helper...");
  Function *main = module->getFunction("main");
  FunctionsHelperWriter::Write(END, DUMP_REGISTERS, module, main);
  FunctionsHelperWriter::Write(NONE, INTERRUPT_PROLOGUE, module, main);
  FunctionsHelperWriter::Write(NONE, INTERRUPT_EPILOGUE, module, main);
  FunctionsHelperWriter::Write(NONE, ICP, module, main);
  FunctionsHelperWriter::Write(NONE, INTERRUPT_HANDLER, module, main);
  // FunctionsHelperWriter::Write(END, DUMP_STACK, module, main);
  inception_message("Done\n");

  // Which IRQ handlers should we patch ?
  // XXX: It could be better if we use the symbols table or the config.json
  // XXX: unfortunately patching inside Compiler does not allow runtime
  // modification of the IRQ vector
  // StringRef handlers[] = {""};
  //
  // // Iterate over each handlers
  // for (auto handler : handlers) {
  //   /*This adds instructions sequence to stacked current context */
  //   InterruptSupport::WriteInterruptPrologue(handler);
  //   InterruptSupport::WriteInterruptEpilogue(handler);
  // }

  std::string bc_output(FileName.str());
  bc_output += ".ll";

  save(bc_output, module);

  return std::error_code();
}

///===---------------------------------------------------------------------===//
/// save - Saves current module to a .ll file
///
static void save(std::string fileName, Module *module) {
  std::error_code ErrorInfo;
  raw_fd_ostream FOut(fileName, ErrorInfo, sys::fs::OpenFlags::F_RW);

  FOut << *module;

  if (ErrorInfo) {
    inception_error("Cannot save result into %s", fileName.c_str());
  }
}

int main(int argc, char *argv[]) {
  inception_message("\n\n");
  inception_message("___________________________________________________");
  inception_message("Inception-compiler v0.1");
  inception_message("\n");
  inception_message("Author  : Camurati Giovanni");
  inception_message("Author  : Corteggiani Nassim");
  inception_message("\n");
  inception_message("Contact : Giovanni.Camurati@eurecom.fr");
  inception_message("Contact : Nassim.Corteggiani@maximintegrated.com");
  inception_message("\n\n");
  inception_message("___________________________________________________");
  inception_message("\n\n");

  ProgramName = argv[0];

  if (ProgramName.find("./") == 0) {
    // Remove the "./" from the beginning of the program name
    ProgramName = ProgramName.substr(2, ProgramName.length() - 2);
  }

  // If no parameter is given to dish, stop execution
  if (argc < 2) {
    // Tell the user how to run the program
    errs() << ProgramName << ": No positional arguments specified!"
           << "\n";
    errs() << "Must specify exactly 1 positional argument: See: ./"
           << ProgramName << " -help"
           << "\n";
    return 1;
  }

  // Stack trace err hdlr
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  // Calls a shutdown function when destructor is called
  llvm_shutdown_obj Y;

  InitializeAllTargetInfos();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllDisassemblers();
  InitializeAllTargets();

  // Register the target printer for --version.
  cl::AddExtraVersionPrinter(TargetRegistry::printRegisteredTargetsForVersion);

  cl::ParseCommandLineOptions(argc, argv, "DIsassembler SHell");

  if (std::error_code Err = runInception(InputBinaryFileName.getValue())) {
    inception_error("Could not open the binary file %s : %s",
                    InputBinaryFileName.getValue().c_str(),
                    Err.message().c_str());
  }

  return 0;
}
