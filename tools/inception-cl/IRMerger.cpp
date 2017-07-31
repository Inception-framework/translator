#include "IRMerger.h"

#include "FunctionsHelperWriter.h"

extern bool nameLookupAddr(StringRef funcName, uint64_t& Address);

namespace fracture {

bool IRMerger::first_call = true;

std::map<std::string, SDNode*> IRMerger::registersNodes = {};

IRMerger::IRMerger(Decompiler* P_DEC, std::string new_function_name)
    : DEC(P_DEC) {
  RegMap.grow(DEC->getDisassembler()
                  ->getMCDirector()
                  ->getMCRegisterInfo()
                  ->getNumRegs());

  function_name = new StringRef(new_function_name);
}

IRMerger::~IRMerger() {
  marked_old_instructions.clear();

  marked_old_basicblocks.clear();
}

void IRMerger::SetNewFunction(std::string new_function_name) {
  function_name = new StringRef(new_function_name);

  marked_old_instructions.clear();

  marked_old_basicblocks.clear();
}

void IRMerger::Run() {
  bool empty = false;

  if (IRMerger::first_call) {
    IRMerger::first_call = false;

    Value* Reg = DEC->getModule()->getGlobalVariable("STACK");
    if (Reg == NULL) {
      Type* Ty = ArrayType::get(
          IntegerType::get(DEC->getModule()->getContext(), 4), 8200);

      Constant* Initializer = Constant::getNullValue(Ty);

      GlobalVariable* gvar_ptr_SP =
          new GlobalVariable(*DEC->getModule(),  // Module
                             Ty,                 // Type
                             false,              // isConstant
                             GlobalValue::CommonLinkage, Initializer, "STACK");
      gvar_ptr_SP->setAlignment(4);
    }
  }

  fct = DEC->getModule()->getFunction(*function_name);

  if (fct->empty()) empty = true;

  if (!empty) {
    entry_bb = &(fct->getEntryBlock());

    MarkOldInstructions();
  } else {
    LLVMContext context;

    entry_bb = BasicBlock::Create(context, "entry", fct);

    fct->addFnAttr("Empty", "True");
  }

  MapArgsToRegs();

  Function* new_function = Decompile();
  if (new_function == NULL) return;

  if (!empty) {
    RemoveUseless();
  }

  FunctionsHelperWriter::Write(END, DUMP_REGISTERS, DEC->getModule());
  FunctionsHelperWriter::Write(BEGIN, INIT_STACK, DEC->getModule());
  // DEC->getModule()->dump();
}

Function* IRMerger::Decompile() {
  uint64_t Address;

  // outs() << "[Inception] The funcion " << *function_name;
  // outs() << " needs to be replaced...\n";

  if (nameLookupAddr(*function_name, Address) == false) {
    return NULL;
  }

  // DEC->setViewMCDAGs(true);
  // DEC->setViewIRDAGs(true);

  formatted_raw_ostream Out(outs(), false);

  DEC->decompile(Address);

  // outs() << "-----------New IR Code --------------------\n";
  // DEC->printInstructions(Out, Address);
  // outs() << "-------------------------------------------\n";

  return DEC->getModule()->getFunction(*function_name);
}

void IRMerger::MarkOldInstructions() {
  for (auto bb_i = fct->begin(); bb_i != fct->end(); bb_i++) {
    BasicBlock& old_bb = *bb_i;

    for (auto int_i = old_bb.begin(); int_i != old_bb.end(); int_i++) {
      Instruction& old_inst = *int_i;

      if (&old_inst != NULL) {
        const UnreachableInst* ui = dyn_cast<UnreachableInst>(&old_inst);
        const ReturnInst* ret = dyn_cast<ReturnInst>(&old_inst);

        if (ui != NULL) {
          marked_old_binstructions.push_back(&old_inst);
        } else if (ret != NULL) {
          marked_old_binstructions.push_back(&old_inst);
        } else
          marked_old_instructions.push_back(&old_inst);
      }
    }  // END INST LOOP

    if (&old_bb != entry_bb) {
      marked_old_basicblocks.push_back(&old_bb);
      // old_bb.dropAllReferences();
      // old_bb.removeFromParent();
      // old_bb.insertInto (fct);
    }
  }  // END BB LOOP
}

void IRMerger::RemoveUseless() {
  BasicBlock* last_bb = NULL;

  for (auto bb_i = fct->begin(); bb_i != fct->end(); bb_i++) last_bb = bb_i;

  unsigned int ret_counter = 0;

  for (auto bb_i = fct->begin(); bb_i != fct->end(); bb_i++) {
    BasicBlock& old_bb = *bb_i;
    for (auto int_i = old_bb.begin(); int_i != old_bb.end(); int_i++) {
      Instruction& old_inst = *int_i;

      SmallVector<std::pair<unsigned, MDNode*>, 100> Metadata;

      int_i->getAllMetadata(Metadata);

      for (unsigned i = 0, n = Metadata.size(); i < n; ++i) {
        unsigned Kind = Metadata[i].first;

        int_i->setMetadata(Kind, nullptr);
      }
      Metadata.clear();

      if (&old_inst != NULL) {
        const ReturnInst* ret = dyn_cast<ReturnInst>(&old_inst);

        if (ret != NULL) {
          marked_old_binstructions.push_back(&old_inst);

          IRBuilder<>* IRB = new IRBuilder<>(&old_inst);

          Type* FType = fct->getReturnType();

          if (FType->isVoidTy()) {
            IRB->CreateRetVoid();

            continue;
          }

          Value* Reg = DEC->getModule()->getGlobalVariable("R0");
          if (Reg == NULL) {
            Type* Ty = IntegerType::get(DEC->getModule()->getContext(), 32);

            Constant* Initializer = Constant::getNullValue(Ty);

            GlobalVariable* gvar_i32 = new GlobalVariable(
                *DEC->getModule(),  // Module
                Ty,                 // Type
                false,              // isConstant
                GlobalValue::CommonLinkage, Initializer, "R0");

            Reg = gvar_i32;

            // outs() << "MISSING REGISTER R0 TO CREATE RETURN INSTRUCTION
            // \n\n"; return;
          }

          if (FType->isPointerTy())
            IRB->CreateRet(Reg);

          if (FType->isIntegerTy()) {
            std::string ret_name = "R0_RET" + std::to_string(ret_counter);
            StringRef ReturnName(ret_name);

            Instruction* Res = IRB->CreateLoad(Reg, ReturnName);

            IRB->CreateRet(Res);

            ret_counter++;
          }
        }
      }
    }
  }

  for (auto& inst : marked_old_instructions) {
    RemoveInstruction(inst);
  }

  for (auto& inst : marked_old_binstructions) {
    RemoveInstruction(inst);
  }

  for (auto& bb : marked_old_basicblocks) {
    bb->dropAllReferences();
    // bb->removeFromParent();
    bb->eraseFromParent();
  }
}

void IRMerger::RemoveInstruction(llvm::Instruction* instruction) {
  SmallVector<std::pair<unsigned, MDNode*>, 100> Metadata;

  instruction->getAllMetadata(Metadata);

  for (unsigned i = 0, n = Metadata.size(); i < n; ++i) {
    unsigned Kind = Metadata[i].first;

    instruction->setMetadata(Kind, nullptr);
  }
  Metadata.clear();

  outs() << "Removing : " << *instruction << "\n";
  instruction->dropAllReferences();
  // instruction->removeFromParent();
  instruction->eraseFromParent();
}

void IRMerger::MapArgsToRegs() {
  // outs() << "=========MAP ARGS===========\n\n";

  BasicBlock& new_entry = fct->getEntryBlock();

  IRBuilder<>* IRB = new IRBuilder<>(&new_entry);

  uint8_t reg_counter = 0;
  for (auto arg = fct->arg_begin(); arg != fct->arg_end(); arg++) {
    Value* x = arg;

    std::string reg_name = "R" + std::to_string(reg_counter);

    Value* Reg = DEC->getModule()->getGlobalVariable(reg_name);
    if (Reg == NULL) {
      // ConstantInt* Ty = ConstantInt::get(DEC->getModule()->getContext(),
      // APInt(32,0));
      // PointerType::get(IntegerType::get( DEC->getModule()->getContext(),
      // 32), 0); std::map<std::string, SDNode*>::iterator it; it =
      // registersNodes.find(reg_name); if (it != registersNodes.end()) {
      //
      //   SDNode* node = it->second;
      //
      //   const RegisterSDNode *R = dyn_cast<RegisterSDNode>(node);
      //
      //   Ty = R->getValueType(0).getTypeForEVT(getGlobalContext());
      // }
      // else
      Type* Ty = IntegerType::get(DEC->getModule()->getContext(), 32);

      Constant* Initializer = Constant::getNullValue(Ty);

      GlobalVariable* gvar_i32 =
          new GlobalVariable(*DEC->getModule(),  // Module
                             Ty,                 // Type
                             false,              // isConstant
                             GlobalValue::CommonLinkage, Initializer, reg_name);
      // gvar_i32->setAlignment(4);

      RegMap[reg_counter] = gvar_i32;

      Reg = gvar_i32;

      // outs() << "\n[Inception]\tAdding new register " << reg_name <<
      // "\n";
    }
    // Reg->dump();

    // if (!Addr->getType()->isPointerTy()) {

    // StringRef AddrName(reg_name+"_FARG_P");

    // Addr = IRB->CreateIntToPtr(Addr, Addr->getType()->getPointerTo(),
    // AddrName); outs() << "\nCast "; Addr->dump();
    // }

    // if ( Reg->getType() != x->getType() ) {
    //   outs() << "Solving type issue \n\n";
    //
    //   outs() << "\nRegType "; Reg->getType()->dump();
    //   outs() << "\nArgType "; x->getType()->dump();
    //   outs() << "\nElementType "; x->getElementType()->dump();
    //
    //   else
    //     outs() << "Unknown type ....\n\n";
    // }

    if (x->getType()->isPointerTy()) {
      continue;

      x = IRB->CreateLoad(x);
      // outs() << "\nGet Ptr value ";
      // x->dump();

      if (Reg->getType() != x->getType()) {
        x = IRB->CreateBitCast(x, Reg->getType());
        // outs() << "\nBitcast ";
        // x->dump();
      }
      // x = IRB->CreatePtrToInt(x, x->getType()->getPointerTo());
    }

    // outs() << "\nRegType ";
    // Reg->getType()->dump();
    // outs() << "\nArgType ";
    // x->getType()->dump();

    Instruction* Res = IRB->CreateStore(x, Reg);
    Res->dump();
    // outs() << "==========================\n\n";

    reg_counter++;
  }
}

}  // namespace fracture
