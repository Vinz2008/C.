#include "../../config.h"

#if ENABLE_CIR

#include "llvm_instructions.h"
#include "llvm.h"
#include "structs.h"
#include "../../CIR/cir.h"
#include "../../abi.h"
#include "../../log.h"
#include "llvm/IR/Function.h"

using namespace llvm;

namespace LLVM {

template <class T>
static std::unique_ptr<T> get_Instruction_from_CIR_Instruction(std::unique_ptr<CIR::Instruction> I){
    std::unique_ptr<T> SubClass;
    T* SubClassPtr = dynamic_cast<T*>(I.get());
    if (!SubClassPtr){
        return nullptr;
    }
    I.release();
    SubClass.reset(SubClassPtr);
    return SubClass;
}

static Function* codegenProto(std::unique_ptr<LLVM::Context>& codegen_context, CIR::FunctionProto proto){
    std::vector<Type *> type_args;
    for (int i = 0; i < proto.args.size(); i++){
        Cpoint_Type arg_type = proto.args.at(i).second;
        if (arg_type.is_just_struct() && is_struct_all_type(arg_type, float_type) && struct_get_number_type(arg_type, float_type) <= 2) {
            type_args.push_back(vector_type_from_struct(arg_type));
        } else {
            if (arg_type.is_just_struct() && should_pass_struct_byval(arg_type)){
                arg_type.is_ptr = true;
            }
            type_args.push_back(get_type_llvm(codegen_context, arg_type));
        }
    }
    bool has_sret = false;
    // TODO : add main type (probably already added)
    if (proto.return_type.is_just_struct()){
        if (should_return_struct_with_ptr(proto.return_type)){
            has_sret = true;
            Cpoint_Type sret_arg_type = proto.return_type;
            sret_arg_type.is_ptr = true;
            type_args.insert(type_args.begin(), get_type_llvm(codegen_context, sret_arg_type));
        }
    }
    Cpoint_Type return_type = (has_sret) ? Cpoint_Type(void_type) : proto.return_type;
    FunctionType* FT = FunctionType::get(get_type_llvm(codegen_context, return_type), type_args, proto.is_variable_number_args);
    // TODO : add code for externs ? (see codegen.cpp) (add is_extern to FunctionProto ?)
    GlobalValue::LinkageTypes linkageType = Function::ExternalLinkage;
    if (proto.is_private_func){
        linkageType = Function::InternalLinkage;
    }
    Function *F = Function::Create(FT, linkageType, proto.name, codegen_context->TheModule.get());
    if (proto.return_type.type == never_type){
        F->addFnAttr(Attribute::NoReturn);
        F->addFnAttr(Attribute::NoUnwind);
    }
    unsigned Idx = 0;
    bool has_added_sret = false;
    for (auto &Arg : F->args()){
        if (has_sret && Idx == 0 && !has_added_sret){
            addArgSretAttribute(Arg, get_type_llvm(codegen_context, proto.return_type));
            Arg.setName("sret_arg"); // needed ? (it was already added, remove this ? TODO)
            Idx = 0;
            has_added_sret = true;
        } else if (proto.args.at(Idx).second.is_just_struct() && should_pass_struct_byval(proto.args.at(Idx).second)){
            Log::Info() << "should_pass_struct_byval arg " << proto.args.at(Idx).first << " : " << proto.args.at(Idx).second << "\n";
            Cpoint_Type arg_type = get_cpoint_type_from_llvm(get_type_llvm(codegen_context, proto.args.at(Idx).second));
            Cpoint_Type by_val_ptr_type = arg_type;
            by_val_ptr_type.is_ptr = true;
            by_val_ptr_type.nb_ptr++;
            Arg.mutateType(get_type_llvm(codegen_context, by_val_ptr_type));
            Arg.addAttr(Attribute::getWithByValType(*codegen_context->TheContext, get_type_llvm(codegen_context, arg_type)));
            Arg.addAttr(Attribute::getWithAlignment(*codegen_context->TheContext, Align(8)));
            Arg.setName(proto.args[Idx++].first);
        } else {
            Arg.setName(proto.args[Idx++].first);
        }
    }
    return F;
}

static Function* getFunction(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR>& fileCIR, std::string Name){
    if (auto* F = codegen_context->TheModule->getFunction(Name)){
        return F;
    }

    auto FI = fileCIR->protos.find(Name);
    if (FI != fileCIR->protos.end()){
        return codegenProto(codegen_context, FI->second);
    }
    // TODO : codegen with function protos ? (see codegen.cpp)

    return nullptr;
}

static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction, StringRef VarName, Type* type){
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, 0, VarName);
}

static void codegenInstruction(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR>& fileCIR, std::unique_ptr<CIR::Instruction>& instruction) {
    Value* instruction_val = nullptr;
    if (dynamic_cast<CIR::CallInstruction*>(instruction.get())){
        // TODO : move this in a function
        auto call_instr = get_Instruction_from_CIR_Instruction<CIR::CallInstruction>(std::move(instruction));
        Function* calling_function = getFunction(codegen_context, fileCIR, call_instr->Callee);
        // TODO : add support for NamedValues calling
        bool has_sret = call_instr->type.is_just_struct() && should_return_struct_with_ptr(call_instr->type);
        std::vector<Value *> Args;
        AllocaInst* SretArgAlloca = nullptr;
        if (has_sret){
            Function* TheFunction = codegen_context->Builder->GetInsertBlock()->getParent();
            SretArgAlloca = CreateEntryBlockAlloca(TheFunction, call_instr->type.struct_name + "_sret", get_type_llvm(codegen_context, call_instr->type));
            int idx = 0;
            for (auto& Arg : calling_function->args()){
                if (idx == 0){
                    addArgSretAttribute(Arg, SretArgAlloca->getAllocatedType());
                } else {
                    break;
                }
                idx++;
            }
            Args.push_back(SretArgAlloca);
        }
        for (int i = 0; i < call_instr->Args.size(); i++){
            Value* arg_val = codegen_context->tempBBValues.at(call_instr->Args.at(i).get_pos());
            // TODO : implement byval and vector detection (see CallExprAST::codegen)
            Args.push_back(arg_val);
        }
        auto call = codegen_context->Builder->CreateCall(calling_function, Args, call_instr->label);
        if (has_sret){
            call->addParamAttr(0, Attribute::getWithStructRetType(*codegen_context->TheContext, SretArgAlloca->getAllocatedType()));
            call->addParamAttr(0, Attribute::getWithAlignment(*codegen_context->TheContext, Align(8))); // TODO : change the align depending of the architecture
            instruction_val = codegen_context->Builder->CreateLoad(SretArgAlloca->getAllocatedType(), SretArgAlloca);
        } else {
            instruction_val = call;
        }
    } else if (dynamic_cast<CIR::ReturnInstruction*>(instruction.get())){
        auto return_instr = get_Instruction_from_CIR_Instruction<CIR::ReturnInstruction>(std::move(instruction));
        instruction_val = codegen_context->Builder->CreateRet(codegen_context->tempBBValues.at(return_instr->ret_val.get_pos()));
    } else if (dynamic_cast<CIR::LoadArgInstruction*>(instruction.get())){
        auto load_arg_instruction = get_Instruction_from_CIR_Instruction<CIR::LoadArgInstruction>(std::move(instruction));
        instruction_val = codegen_context->Builder->CreateLoad(get_type_llvm(codegen_context, load_arg_instruction->type), codegen_context->tempVarsAllocas[load_arg_instruction->arg_name]);
    } else if (dynamic_cast<CIR::ConstNumber*>(instruction.get())){
        auto const_nb_instruction = get_Instruction_from_CIR_Instruction<CIR::ConstNumber>(std::move(instruction));
        if (const_nb_instruction->is_float){
            instruction_val = ConstantFP::get(*codegen_context->TheContext, APFloat(const_nb_instruction->nb_val.float_nb));
        } else {
            instruction_val = ConstantInt::get(*codegen_context->TheContext, APInt(const_nb_instruction->type.get_number_of_bits(), const_nb_instruction->nb_val.int_nb, const_nb_instruction->type.is_signed()));
        }
    } else if (dynamic_cast<CIR::ConstBool*>(instruction.get())){
        auto const_bool_instruction = get_Instruction_from_CIR_Instruction<CIR::ConstBool>(std::move(instruction));
        instruction_val = ConstantInt::get(*codegen_context->TheContext, APInt(1, (uint64_t)const_bool_instruction->val, false));
    } else if (dynamic_cast<CIR::CastInstruction*>(instruction.get())){
        auto cast_instruction = get_Instruction_from_CIR_Instruction<CIR::CastInstruction>(std::move(instruction));
        Value* cast_val = codegen_context->tempBBValues.at(cast_instruction->val.get_pos());
        convert_to_type(cast_instruction->type, cast_instruction->cast_type, cast_val);
        instruction_val = cast_val;
    } else if (dynamic_cast<CIR::VarInit*>(instruction.get())){
        auto var_init_instruction = get_Instruction_from_CIR_Instruction<CIR::VarInit>(std::move(instruction));
        Function* TheFunction = codegen_context->Builder->GetInsertBlock()->getParent();
        AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, var_init_instruction->VarName.first, get_type_llvm(codegen_context, var_init_instruction->type));
        if (!var_init_instruction->VarName.second.is_empty()){
            Value* InitVal = codegen_context->tempBBValues.at(var_init_instruction->VarName.second.get_pos());
            codegen_context->Builder->CreateStore(InitVal, Alloca);
        }
        codegen_context->tempVarsAllocas[var_init_instruction->VarName.first] = Alloca;
        instruction_val = Alloca;
        //instruction_val = nullptr;
    } else if (dynamic_cast<CIR::LoadGlobalInstruction*>(instruction.get())){
        auto load_global_instruction = get_Instruction_from_CIR_Instruction<CIR::LoadGlobalInstruction>(std::move(instruction));
        if (load_global_instruction->is_string){
            instruction_val = codegen_context->staticStrings.at(load_global_instruction->global_pos);
        } else {
            fprintf(stderr, "NOT IMPLEMENTED, TODO");
            exit(1);
        }
    } else if (dynamic_cast<CIR::LoadVarInstruction*>(instruction.get())){
        auto load_var_instruction = get_Instruction_from_CIR_Instruction<CIR::LoadVarInstruction>(std::move(instruction));
        Value* AllocaVal = codegen_context->tempBBValues.at(load_var_instruction->var.get_pos());
        instruction_val = codegen_context->Builder->CreateLoad(get_type_llvm(codegen_context, load_var_instruction->load_type), AllocaVal);
    } else if (dynamic_cast<CIR::CmpInstruction*>(instruction.get())){
        auto cmp_instruction = get_Instruction_from_CIR_Instruction<CIR::CmpInstruction>(std::move(instruction));
        Value* arg1_val = codegen_context->tempBBValues.at(cmp_instruction->arg1.get_pos());
        Value* arg2_val = codegen_context->tempBBValues.at(cmp_instruction->arg2.get_pos());
        Type* arg_type = arg1_val->getType();
        std::string cmp_label = "cmp_tmp"; // TODO : move the label to the cir part
        if (arg_type->isVectorTy()){
            arg_type = dyn_cast<VectorType>(arg_type)->getElementType();
        }
        switch (cmp_instruction->cmp_type){
            case CIR::CmpInstruction::CMP_EQ:
                if (arg_type->isFloatTy()){
                    instruction_val =  codegen_context->Builder->CreateFCmpUEQ(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpEQ(arg1_val, arg2_val, cmp_label);
                }
                break;
            case CIR::CmpInstruction::CMP_NOT_EQ:
                if (arg_type->isFloatTy()){
                    instruction_val = codegen_context->Builder->CreateFCmpUNE(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpNE(arg1_val, arg2_val, cmp_label);
                }
                break;
            case CIR::CmpInstruction::CMP_GREATER:
                if (arg_type->isFloatTy()){
                    instruction_val = codegen_context->Builder->CreateFCmpOGT(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpSGT(arg1_val,  arg2_val, cmp_label);
                }
                break;
            case CIR::CmpInstruction::CMP_GREATER_EQ:
                if (arg_type->isFloatTy()){
                    instruction_val = codegen_context->Builder->CreateFCmpOGE(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpSGE(arg1_val, arg2_val, cmp_label);
                }
                break;
            case CIR::CmpInstruction::CMP_LOWER:
                arg1_val->print(outs());
                arg2_val->print(outs());
                if (arg_type->isFloatTy()){
                    instruction_val = codegen_context->Builder->CreateFCmpOLT(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpSLT(arg1_val, arg2_val, cmp_label);
                }
                break;
            case CIR::CmpInstruction::CMP_LOWER_EQ:
                if (arg_type->isFloatTy()){
                    instruction_val = codegen_context->Builder->CreateFCmpOLE(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpSLE(arg1_val, arg2_val, cmp_label);
                }
                break;
            default:
                fprintf(stderr, "CIR ERROR : Invalid compare instruction");
                exit(1);
        }
    } else if (dynamic_cast<CIR::ConstVoid*>(instruction.get())){
        instruction_val = nullptr;
    } else {
        fprintf(stderr, "ERROR : unknown instruction %s\n", typeid(*instruction.get()).name());
        exit(1);
    }
    codegen_context->tempBBValues.push_back(instruction_val);
}

static void codegenBasicBlock(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR> &fileCIR, Function* TheFunction, std::unique_ptr<CIR::BasicBlock> basic_block, bool is_first_bb = false){
    if (!is_first_bb){
        BasicBlock *BB = BasicBlock::Create(*codegen_context->TheContext, basic_block->name, TheFunction);
        codegen_context->Builder->SetInsertPoint(BB);
    }
    codegen_context->tempBBValues.clear();
    for (int i = 0; i < basic_block->instructions.size(); i++){
        codegenInstruction(codegen_context, fileCIR, basic_block->instructions.at(i));
    }
}

static Function* codegenFunction(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR> &fileCIR, std::unique_ptr<CIR::Function> function) {
    Function *TheFunction = getFunction(codegen_context, fileCIR, function->proto->name);
    // TODO : uncomment this after implementing codegen of instructions
    bool has_sret = function->proto->return_type.is_just_struct() && should_return_struct_with_ptr(function->proto->return_type);
    codegen_context->tempVarsAllocas.clear();
    codegen_context->tempBBValues.clear();
    BasicBlock* entryBB = BasicBlock::Create(*codegen_context->TheContext, function->basicBlocks.at(0)->name, TheFunction);
    codegen_context->Builder->SetInsertPoint(entryBB);
    int j = 0;
    for (auto &Arg : TheFunction->args()){
        std::string arg_name;
        Type* arg_type;
        if (has_sret && j == 0){
            arg_name = "sret_arg";
            arg_type = get_type_llvm(codegen_context, Cpoint_Type(void_type, true));
        } else {
            auto func_arg = function->proto->args.at(j);
            arg_name = func_arg.first;
            arg_type = get_type_llvm(codegen_context, func_arg.second);
        }
        auto Alloca = CreateEntryBlockAlloca(TheFunction, arg_name, arg_type);
        //auto llvm_arg = std::next(TheFunction->args().begin(), j);
        codegen_context->Builder->CreateStore(&Arg, Alloca);
        codegen_context->tempVarsAllocas[arg_name] = Alloca;
        j++;
    }
    for (int i = 0; i < function->basicBlocks.size(); i++){
        codegenBasicBlock(codegen_context, fileCIR, TheFunction, std::move(function->basicBlocks.at(i)), i == 0);
    }
    // TODO : add memcpy for sret
    return TheFunction;
}

void codegenFile(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR> fileCIR){
    for (int i = 0; i < fileCIR->strings.size(); i++){
        auto const_str = codegen_context->Builder->CreateGlobalStringPtr(StringRef(fileCIR->strings.at(i)), "", 0, codegen_context->TheModule.get());
        codegen_context->staticStrings.push_back(const_str);
    }
    for (auto& s : fileCIR->structs){
        codegenStruct(codegen_context, s.second);
    }
    for (int i = 0; i < fileCIR->functions.size(); i++){
        codegenFunction(codegen_context, fileCIR, std::move(fileCIR->functions.at(i)));
    }
}

}

#endif