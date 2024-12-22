#include "../../config.h"

#if ENABLE_CIR
//#if 1

#include "../../cpoint.h"
#include "llvm_instructions.h"
#include "llvm.h"
#include "structs.h"
#include "../../CIR/cir.h"
#include "../../abi.h"
#include "../../log.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/InlineAsm.h"

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
            addArgSretAttribute(*codegen_context->TheContext, Arg, get_type_llvm(codegen_context, proto.return_type));
            Arg.setName("sret_arg"); // needed ? (it was already added, remove this ? TODO)
            Idx = 0;
            has_added_sret = true;
        } else if (proto.args.at(Idx).second.is_just_struct() && should_pass_struct_byval(proto.args.at(Idx).second)){
            Log::Info() << "should_pass_struct_byval arg " << proto.args.at(Idx).first << " : " << proto.args.at(Idx).second << "\n";
            Cpoint_Type arg_type = proto.args.at(Idx).second;
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

// TODO : remove ref to instruction (because it is technically moved ?)

static void codegenInstruction(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR>& fileCIR, std::unique_ptr<CIR::Instruction> instruction, bool is_global = false) {
    std::string instruction_label = instruction->label;
    Value* instruction_val = nullptr;
    if (dynamic_cast<CIR::CallInstruction*>(instruction.get())){
        // TODO : move this in a function
        std::string internal_func_prefix = INTERNAL_FUNC_PREFIX;
        auto call_instr = get_Instruction_from_CIR_Instruction<CIR::CallInstruction>(std::move(instruction));
        Function* calling_function = getFunction(codegen_context, fileCIR, call_instr->Callee);
        // TODO : add support for NamedValues calling
        bool should_add_unreachable = call_instr->type.type == never_type;
        bool has_sret = call_instr->type.is_just_struct() && should_return_struct_with_ptr(call_instr->type);
        std::vector<Value *> Args;
        AllocaInst* SretArgAlloca = nullptr;
        if (has_sret){
            //Function* TheFunction = codegen_context->Builder->GetInsertBlock()->getParent();
            SretArgAlloca = CreateEntryBlockAlloca(codegen_context->TheFunction, call_instr->type.struct_name + "_sret", get_type_llvm(codegen_context, call_instr->type));
            int idx = 0;
            for (auto& Arg : calling_function->args()){
                if (idx == 0){
                    addArgSretAttribute(*codegen_context->TheContext, Arg, SretArgAlloca->getAllocatedType());
                } else {
                    break;
                }
                idx++;
            }
            Args.push_back(SretArgAlloca);

        }
        for (int i = 0; i < call_instr->Args.size(); i++){
            Value* arg_val = codegen_context->functionValues.at(call_instr->Args.at(i).get_pos());
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
        if (should_add_unreachable){
            codegen_context->Builder->CreateUnreachable();
        }
    } else if (dynamic_cast<CIR::ReturnInstruction*>(instruction.get())){
        auto return_instr = get_Instruction_from_CIR_Instruction<CIR::ReturnInstruction>(std::move(instruction));
        instruction_val = codegen_context->Builder->CreateRet(codegen_context->functionValues.at(return_instr->ret_val.get_pos()));
    } else if (dynamic_cast<CIR::InitArgInstruction*>(instruction.get())){
        // create the arg at the start of the function frame
        auto init_arg_instruction = get_Instruction_from_CIR_Instruction<CIR::InitArgInstruction>(std::move(instruction));
        AllocaInst* Alloca = codegen_context->functionVarsAllocas[init_arg_instruction->arg_name];
        //Type* load_type = get_type_llvm(codegen_context, load_arg_instruction->type);
        //Type* load_type = Alloca->getAllocatedType();
        //instruction_val = codegen_context->Builder->CreateLoad(load_type, Alloca);
        instruction_val = Alloca;
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
        Value* cast_val = codegen_context->functionValues.at(cast_instruction->val.get_pos());
        convert_to_type(codegen_context, cast_instruction->from_type, cast_instruction->cast_type, cast_val);
        instruction_val = cast_val;
    } else if (dynamic_cast<CIR::VarInit*>(instruction.get())){
        auto var_init_instruction = get_Instruction_from_CIR_Instruction<CIR::VarInit>(std::move(instruction));
        //Function* TheFunction = codegen_context->Builder->GetInsertBlock()->getParent();
        AllocaInst *Alloca = CreateEntryBlockAlloca(codegen_context->TheFunction, var_init_instruction->VarName.first, get_type_llvm(codegen_context, var_init_instruction->type));
        if (!var_init_instruction->VarName.second.is_empty()){
            Value* InitVal = codegen_context->functionValues.at(var_init_instruction->VarName.second.get_pos());
            codegen_context->Builder->CreateStore(InitVal, Alloca);
        }
        codegen_context->functionVarsAllocas[var_init_instruction->VarName.first] = Alloca;
        instruction_val = Alloca;
        //instruction_val = nullptr;
    } else if (dynamic_cast<CIR::LoadGlobalInstruction*>(instruction.get())){
        auto load_global_instruction = get_Instruction_from_CIR_Instruction<CIR::LoadGlobalInstruction>(std::move(instruction));
        if (load_global_instruction->is_string){
            instruction_val = codegen_context->staticStrings.at(load_global_instruction->global_pos);
        } else {
            auto global_var = codegen_context->GlobalVars[load_global_instruction->var_name];
            instruction_val = codegen_context->Builder->CreateLoad(global_var->getValueType(), global_var, load_global_instruction->label);
            //fprintf(stderr, "NOT IMPLEMENTED, TODO\n");
            //exit(1);
        }
    } else if (dynamic_cast<CIR::StoreGlobalInstruction*>(instruction.get())){
        auto store_global_instruction = get_Instruction_from_CIR_Instruction<CIR::StoreGlobalInstruction>(std::move(instruction));
        Value* store_val = codegen_context->functionValues.at(store_global_instruction->val.get_pos());
        Value* store_var = codegen_context->GlobalVars[store_global_instruction->var_name];
        codegen_context->Builder->CreateStore(store_val, store_var);
        instruction_val = nullptr;
    } else if (dynamic_cast<CIR::LoadVarInstruction*>(instruction.get())){
        auto load_var_instruction = get_Instruction_from_CIR_Instruction<CIR::LoadVarInstruction>(std::move(instruction));
        Value* AllocaVal = codegen_context->functionValues.at(load_var_instruction->var.get_pos());
        instruction_val = codegen_context->Builder->CreateLoad(get_type_llvm(codegen_context, load_var_instruction->load_type), AllocaVal);
    } else if (dynamic_cast<CIR::StoreVarInstruction*>(instruction.get())){
        auto store_var_instruction = get_Instruction_from_CIR_Instruction<CIR::StoreVarInstruction>(std::move(instruction));
        Value* store_val = codegen_context->functionValues.at(store_var_instruction->val.get_pos());
        Value* store_var = codegen_context->functionValues.at(store_var_instruction->var.get_pos());
        codegen_context->Builder->CreateStore(store_val, store_var);
        instruction_val = nullptr;
    } else if (dynamic_cast<CIR::CmpInstruction*>(instruction.get())){
        auto cmp_instruction = get_Instruction_from_CIR_Instruction<CIR::CmpInstruction>(std::move(instruction));
        Value* arg1_val = codegen_context->functionValues.at(cmp_instruction->arg1.get_pos());
        Value* arg2_val = codegen_context->functionValues.at(cmp_instruction->arg2.get_pos());
        Type* arg_type = arg1_val->getType();
        std::string cmp_label = "cmp_tmp"; // TODO : move the label to the cir part
        if (arg_type->isVectorTy()){
            arg_type = dyn_cast<VectorType>(arg_type)->getElementType();
        }
        /*arg1_val->print(outs());
        outs() << " and ";
        arg2_val->print(outs());
        outs() << "\n";
        arg1_val->getType()->print(outs());
        outs() << " and ";
        arg2_val->getType()->print(outs());
        outs() << "\n";
        Type* arg1_type = arg1_val->getType();
        Type* arg2_type = arg2_val->getType();
        outs() << "arg1_val->getType() == arg2_val->getType() : " << (arg1_type == arg2_type) << "\n";*/
        switch (cmp_instruction->cmp_type){
            case CIR::CmpInstruction::CMP_EQ:
                if (arg_type->isFloatingPointTy()){
                    instruction_val =  codegen_context->Builder->CreateFCmpUEQ(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpEQ(arg1_val, arg2_val, cmp_label);
                }
                break;
            case CIR::CmpInstruction::CMP_NOT_EQ:
                if (arg_type->isFloatingPointTy()){
                    instruction_val = codegen_context->Builder->CreateFCmpUNE(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpNE(arg1_val, arg2_val, cmp_label);
                }
                break;
            case CIR::CmpInstruction::CMP_GREATER:
                if (arg_type->isFloatingPointTy()){
                    instruction_val = codegen_context->Builder->CreateFCmpOGT(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpSGT(arg1_val,  arg2_val, cmp_label);
                }
                break;
            case CIR::CmpInstruction::CMP_GREATER_EQ:
                if (arg_type->isFloatingPointTy()){
                    instruction_val = codegen_context->Builder->CreateFCmpOGE(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpSGE(arg1_val, arg2_val, cmp_label);
                }
                break;
            case CIR::CmpInstruction::CMP_LOWER:
                if (arg_type->isFloatingPointTy()){
                    instruction_val = codegen_context->Builder->CreateFCmpOLT(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpSLT(arg1_val, arg2_val, cmp_label);
                }
                break;
            case CIR::CmpInstruction::CMP_LOWER_EQ:
                if (arg_type->isFloatingPointTy()){
                    instruction_val = codegen_context->Builder->CreateFCmpOLE(arg1_val, arg2_val, cmp_label);
                } else {
                    instruction_val = codegen_context->Builder->CreateICmpSLE(arg1_val, arg2_val, cmp_label);
                }
                break;
            default:
                fprintf(stderr, "CIR ERROR : Invalid compare instruction");
                exit(1);
        }

    } else if (dynamic_cast<CIR::AddInstruction*>(instruction.get())){
        auto add_instruction = get_Instruction_from_CIR_Instruction<CIR::AddInstruction>(std::move(instruction));
        Value* arg1_val = codegen_context->functionValues.at(add_instruction->arg1.get_pos());
        Value* arg2_val = codegen_context->functionValues.at(add_instruction->arg2.get_pos());
        Cpoint_Type add_type = add_instruction->type;
        if (!add_type.is_decimal_number_type() || add_type.is_vector_type){ // TODO : maybe use fadd when the type contained in the vector is a float
            instruction_val = codegen_context->Builder->CreateAdd(arg1_val, arg2_val, "addtmp");
        } else {
            instruction_val = codegen_context->Builder->CreateFAdd(arg1_val, arg2_val, "faddtmp");
        }
    } else if (dynamic_cast<CIR::SubInstruction*>(instruction.get())){
        auto sub_instruction = get_Instruction_from_CIR_Instruction<CIR::SubInstruction>(std::move(instruction));
        Value* arg1_val = codegen_context->functionValues.at(sub_instruction->arg1.get_pos());
        Value* arg2_val = codegen_context->functionValues.at(sub_instruction->arg2.get_pos());
        Cpoint_Type sub_type = sub_instruction->type;
        if (!sub_type.is_decimal_number_type() || sub_type.is_vector_type){
            instruction_val = codegen_context->Builder->CreateSub(arg1_val, arg2_val, "subtmp");
        } else {
            instruction_val = codegen_context->Builder->CreateFSub(arg1_val, arg2_val, "fsubtmp");
        }
    } else if (dynamic_cast<CIR::MulInstruction*>(instruction.get())){
        auto mul_instruction = get_Instruction_from_CIR_Instruction<CIR::MulInstruction>(std::move(instruction));
        Value* arg1_val = codegen_context->functionValues.at(mul_instruction->arg1.get_pos());
        Value* arg2_val = codegen_context->functionValues.at(mul_instruction->arg2.get_pos());
        Cpoint_Type mul_type = mul_instruction->type;
        if (!mul_type.is_decimal_number_type() || mul_type.is_vector_type){
            instruction_val = codegen_context->Builder->CreateMul(arg1_val, arg2_val, "multmp");
        } else {
            instruction_val = codegen_context->Builder->CreateFMul(arg1_val, arg2_val, "fmultmp");
        }
    } else if (dynamic_cast<CIR::DivInstruction*>(instruction.get())){
        auto div_instruction = get_Instruction_from_CIR_Instruction<CIR::DivInstruction>(std::move(instruction));
        Value* arg1_val = codegen_context->functionValues.at(div_instruction->arg1.get_pos());
        Value* arg2_val = codegen_context->functionValues.at(div_instruction->arg2.get_pos());
        Cpoint_Type div_type = div_instruction->type;
        if (!div_type.is_decimal_number_type() || div_type.is_vector_type){
            if (div_type.is_signed()){
                instruction_val = codegen_context->Builder->CreateSDiv(arg1_val, arg2_val, "divtmp");
            } else {
                instruction_val = codegen_context->Builder->CreateUDiv(arg1_val, arg2_val, "divtmp");
            }
        } else {
            instruction_val = codegen_context->Builder->CreateFDiv(arg1_val, arg2_val, "fdivtmp");
        }
    } else if (dynamic_cast<CIR::RemInstruction*>(instruction.get())){
        auto rem_instruction = get_Instruction_from_CIR_Instruction<CIR::RemInstruction>(std::move(instruction));
        Value* arg1_val = codegen_context->functionValues.at(rem_instruction->arg1.get_pos());
        Value* arg2_val = codegen_context->functionValues.at(rem_instruction->arg2.get_pos());
        Cpoint_Type rem_type = rem_instruction->type;
        if (!rem_type.is_decimal_number_type()){
            if (rem_type.is_signed()){
                instruction_val = codegen_context->Builder->CreateSRem(arg1_val, arg2_val, "sremtmp");
            } else {
                instruction_val = codegen_context->Builder->CreateURem(arg1_val, arg2_val, "uremtmp");
            }
        } else {
            instruction_val = codegen_context->Builder->CreateFRem(arg1_val, arg2_val, "fremtmp");
        }
    } else if (dynamic_cast<CIR::AndInstruction*>(instruction.get())){
        auto and_instruction = get_Instruction_from_CIR_Instruction<CIR::AndInstruction>(std::move(instruction));
        Value* arg1_val = codegen_context->functionValues.at(and_instruction->arg1.get_pos());
        Value* arg2_val = codegen_context->functionValues.at(and_instruction->arg2.get_pos());
        instruction_val = codegen_context->Builder->CreateAnd(arg1_val, arg2_val, "andtmp");
    } else if (dynamic_cast<CIR::OrInstruction*>(instruction.get())){
        auto or_instruction = get_Instruction_from_CIR_Instruction<CIR::OrInstruction>(std::move(instruction));
        Value* arg1_val = codegen_context->functionValues.at(or_instruction->arg1.get_pos());
        Value* arg2_val = codegen_context->functionValues.at(or_instruction->arg2.get_pos());
        Cpoint_Type or_type = or_instruction->type;
        instruction_val = codegen_context->Builder->CreateOr(arg1_val, arg2_val, "andtmp");
    } else if (dynamic_cast<CIR::GotoInstruction*>(instruction.get())){
        auto goto_instruction = get_Instruction_from_CIR_Instruction<CIR::GotoInstruction>(std::move(instruction));
        //BasicBlock* bb = codegen_context->functionBBs.at(goto_instruction->goto_bb.get_pos());
        BasicBlock* bb = codegen_context->get_function_BB(goto_instruction->goto_bb);
        codegen_context->Builder->CreateBr(bb);
        instruction_val = nullptr;
    } else if (dynamic_cast<CIR::GotoIfInstruction*>(instruction.get())){
        auto goto_if_instruction = get_Instruction_from_CIR_Instruction<CIR::GotoIfInstruction>(std::move(instruction));
        Value* CondV = codegen_context->functionValues.at(goto_if_instruction->cond_instruction.get_pos());
        BasicBlock* bb_if_true = codegen_context->get_function_BB(goto_if_instruction->goto_bb_if_true);
        BasicBlock* bb_if_false = codegen_context->get_function_BB(goto_if_instruction->goto_bb_if_false);
        codegen_context->Builder->CreateCondBr(CondV, bb_if_true, bb_if_false);
        instruction_val = nullptr;
    } else if (dynamic_cast<CIR::PhiInstruction*>(instruction.get())){
        auto phi_instruction = get_Instruction_from_CIR_Instruction<CIR::PhiInstruction>(std::move(instruction));
        Value* phi_val1 = codegen_context->functionValues.at(phi_instruction->arg1.get_pos());
        BasicBlock* phi_bb1 = codegen_context->get_function_BB(phi_instruction->bb1);
        Value* phi_val2 = codegen_context->functionValues.at(phi_instruction->arg2.get_pos());
        BasicBlock* phi_bb2 = codegen_context->get_function_BB(phi_instruction->bb2);
        PHINode* PN = codegen_context->Builder->CreatePHI(get_type_llvm(codegen_context, phi_instruction->phi_type), 2, phi_instruction->label);
        std::cout << "phi_instruction->phi_type : " << phi_instruction->phi_type << std::endl;
        phi_val1->print(outs());
        phi_val2->print(outs());
        PN->addIncoming(phi_val1, phi_bb1);
        PN->addIncoming(phi_val2, phi_bb2);
        instruction_val = PN;
    } else if (dynamic_cast<CIR::SizeofInstruction*>(instruction.get())){
        auto sizeof_instruction = get_Instruction_from_CIR_Instruction<CIR::SizeofInstruction>(std::move(instruction));
        // TODO : compile time sizeof ?
        auto one = llvm::ConstantInt::get(*codegen_context->TheContext, llvm::APInt(32, 1, true));
        Type* llvm_type = nullptr;
        if (sizeof_instruction->is_type){
            llvm_type = get_type_llvm(codegen_context, sizeof_instruction->type);
        } else {
            llvm_type = codegen_context->functionValues.at(sizeof_instruction->expr.get_pos())->getType();
        }
        Value* size = codegen_context->Builder->CreateGEP(llvm_type, codegen_context->Builder->CreateIntToPtr(ConstantInt::get(get_type_llvm(codegen_context, Cpoint_Type(i64_type)), 0),llvm_type->getPointerTo()), {one});
        size = codegen_context->Builder->CreatePtrToInt(size, get_type_llvm(codegen_context, Cpoint_Type(i32_type)));
        instruction_val = size;
    } else if (dynamic_cast<CIR::DerefInstruction*>(instruction.get())){
        auto deref_instruction = get_Instruction_from_CIR_Instruction<CIR::DerefInstruction>(std::move(instruction));
        Type* element_type = get_type_llvm(codegen_context, deref_instruction->element_type);
        Value* VarAlloc = codegen_context->functionValues.at(deref_instruction->ptr.get_pos());
        instruction_val = codegen_context->Builder->CreateLoad(element_type, VarAlloc, deref_instruction->label);
    } else if (dynamic_cast<CIR::GepArray*>(instruction.get())){
        auto array_element_instruction = get_Instruction_from_CIR_Instruction<CIR::GepArray>(std::move(instruction));
        /*if (array_element_instruction->is_array_access_mem){
            NOT_IMPLEMENTED();
        }*/
        Value* array = codegen_context->functionValues.at(array_element_instruction->array.get_pos());
        Value* IndexV = codegen_context->functionValues.at(array_element_instruction->index.get_pos());
        /*if (IndexV->getType() != get_type_llvm(Cpoint_Type(i64_type))){ // TODO : only do this in 64bits target ?
            convert_to_type(codegen_context, get_cpoint_type_from_llvm(IndexV->getType()), Cpoint_Type(i64_type), IndexV); // TODO : remove get_cpoint_type_from_llvm ?
        }*/
        Cpoint_Type array_type = array_element_instruction->array_type;
        //Cpoint_Type element_type = array_element_instruction->element_type; // = ? // TODO
        std::vector<llvm::Value *> IdxList; // TODO
        if (array_type.is_ptr && !array_type.is_array){
            IdxList = {IndexV};
        } else {
            auto zero = llvm::ConstantInt::get(*codegen_context->TheContext, llvm::APInt(64, 0, true));
            IdxList = { zero, IndexV};
        }
        //Type* element_type_llvm = get_type_llvm(codegen_context, element_type);
        auto gep_ptr = codegen_context->Builder->CreateGEP(/*element_type_llvm*/ get_type_llvm(codegen_context, array_type), array, IdxList, "", true); // TODO : then load ?
        instruction_val = gep_ptr;
        //instruction_val = codegen_context->Builder->CreateLoad(element_type_llvm, gep_ptr);
    } else if (dynamic_cast<CIR::GepStruct*>(instruction.get())){
        auto struct_element_instruction = get_Instruction_from_CIR_Instruction<CIR::GepStruct>(std::move(instruction));


        Value* struct_val = codegen_context->functionValues.at(struct_element_instruction->struct_ref.get_pos());
        Cpoint_Type struct_type = struct_element_instruction->struct_type;
        int pos = 0; // TODO
        auto zero = llvm::ConstantInt::get(*codegen_context->TheContext, llvm::APInt(32, 0, true));
        auto index = llvm::ConstantInt::get(*codegen_context->TheContext, llvm::APInt(32, pos, true)); 
        std::vector<llvm::Value *> IdxList = { zero, index};
        auto gep_ptr = codegen_context->Builder->CreateGEP(get_type_llvm(codegen_context, struct_type), struct_val, IdxList, "", true); // TODO : then load ?
        instruction_val = gep_ptr;
    } else if (dynamic_cast<CIR::StoreInPtr*>(instruction.get())){
        auto store_in_ptr_instruction = get_Instruction_from_CIR_Instruction<CIR::StoreInPtr>(std::move(instruction));
        Value* store_ptr = codegen_context->functionValues.at(store_in_ptr_instruction->ptr.get_pos());
        Value* store_val = codegen_context->functionValues.at(store_in_ptr_instruction->ptr.get_pos());
        codegen_context->Builder->CreateStore(store_ptr, store_val);
        instruction_val = nullptr;
    } else if (dynamic_cast<CIR::InlineAsmInstruction*>(instruction.get())){
        auto inline_asm_instruction = get_Instruction_from_CIR_Instruction<CIR::InlineAsmInstruction>(std::move(instruction));
        bool contains_out = false;
        std::vector<Value*> AsmArgs;
        std::vector<Type*> AsmArgsTypes;
        auto asm_type = Cpoint_Type(void_type);
        Value* out_var_allocation = nullptr; // for now only support one out var (TODO : will need to extract all of them returned by the  struct instruction

        std::string generated_assembly_code = ""; // TODO : replace {}s
        std::string constraints = ""; // TODO 

        NOT_IMPLEMENTED();

        auto inlineAsm = InlineAsm::get(FunctionType::get(get_type_llvm(codegen_context, asm_type), AsmArgsTypes, false), (StringRef)generated_assembly_code, (StringRef)constraints, true, true, InlineAsm::AD_Intel); // use intel dialect
        if (contains_out){
            auto asmCalled = codegen_context->Builder->CreateCall(inlineAsm, AsmArgs); 
            codegen_context->Builder->CreateStore(asmCalled, out_var_allocation);
        } else {
            codegen_context->Builder->CreateCall(inlineAsm, AsmArgs);
        }
        instruction_val = nullptr;
    } else if (dynamic_cast<CIR::ConstVoid*>(instruction.get())){
        instruction_val = nullptr;
    } else if (dynamic_cast<CIR::ConstNull*>(instruction.get())){
        instruction_val = ConstantPointerNull::get(PointerType::get(*codegen_context->TheContext, 0));
    } else if (dynamic_cast<CIR::ConstNever*>(instruction.get())){
        instruction_val = codegen_context->Builder->CreateUnreachable();
    } else {
        fprintf(stderr, "ERROR (LLVM BACKEND) : unknown instruction %s\n", typeid(*instruction.get()).name());
        exit(1);
    }
    if (instruction_val && instruction_label != ""){
        instruction_val->setName(instruction_label);
    }

    if (is_global){
        codegen_context->GlobalValues.push_back(instruction_val);
    } else {
        codegen_context->functionValues.push_back(instruction_val);
    }
}

// copy of the one in codegen.cpp
static Value* getSizeOfStruct(std::unique_ptr<LLVM::Context> &codegen_context, Type* struct_type, Value *A){
    Type* llvm_type = A->getType();
    auto one = llvm::ConstantInt::get(*codegen_context->TheContext, llvm::APInt(32, 1, true));
    Value* size = codegen_context->Builder->CreateGEP(struct_type, codegen_context->Builder->CreateIntToPtr(ConstantInt::get(codegen_context->Builder->getInt64Ty(), 0), llvm_type->getPointerTo()), {one});
    size = codegen_context->Builder->CreatePtrToInt(size, get_type_llvm(codegen_context, Cpoint_Type(i32_type)));
    return size;
}

static void handleSretReturn(std::unique_ptr<LLVM::Context> &codegen_context, std::unique_ptr<FileCIR> &fileCIR, std::unique_ptr<CIR::Instruction>& instruction){
    std::unique_ptr<CIR::ReturnInstruction> return_instruction = get_Instruction_from_CIR_Instruction<CIR::ReturnInstruction>(std::move(instruction));
    Value* ret_val = codegen_context->functionValues.at(return_instruction->ret_val.get_pos());
    std::string sret_arg_name = "sret_arg";
    AllocaInst* sret_ptr = codegen_context->functionVarsAllocas[sret_arg_name];
    AllocaInst* temp_var = CreateEntryBlockAlloca(codegen_context->TheFunction, "temp_retvar_sret", ret_val->getType());
    codegen_context->Builder->CreateStore(ret_val, temp_var);
    codegen_context->Builder->CreateMemCpy(sret_ptr, MaybeAlign(0), temp_var, MaybeAlign(0), getSizeOfStruct(codegen_context, codegen_context->function_sret_type, sret_ptr));
    codegen_context->Builder->CreateRetVoid();
}

static void codegenBasicBlock(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR> &fileCIR, std::unique_ptr<CIR::BasicBlock> basic_block, int bb_nb, bool has_function_sret){
    if (codegen_context->bb_codegen_number != 0){
        codegen_context->Builder->SetInsertPoint(codegen_context->functionBBs.at(codegen_context->bb_codegen_number).second);
    }
    for (int i = 0; i < basic_block->instructions.size(); i++){
        bool is_last_bb = codegen_context->bb_codegen_number == bb_nb-1;
        bool is_last_instruction = i == basic_block->instructions.size()-1;
        if (has_function_sret && is_last_bb && is_last_instruction && dynamic_cast<CIR::ReturnInstruction*>(basic_block->instructions.at(i).get())){
            handleSretReturn(codegen_context, fileCIR, basic_block->instructions.at(i));
        } else {
            codegenInstruction(codegen_context, fileCIR, std::move(basic_block->instructions.at(i)));
        }
    }
    codegen_context->bb_codegen_number++;
}

static Function* codegenFunction(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR> &fileCIR, std::unique_ptr<CIR::Function> function) {
    Function *TheFunction = getFunction(codegen_context, fileCIR, function->proto->name);
    codegen_context->TheFunction = TheFunction;
    // TODO : uncomment this after implementing codegen of instructions
    bool has_sret = function->proto->return_type.is_just_struct() && should_return_struct_with_ptr(function->proto->return_type);
    if (has_sret){
        codegen_context->function_sret_type = get_type_llvm(codegen_context, function->proto->return_type);
    }
    codegen_context->functionVarsAllocas.clear();
    codegen_context->functionBBs.clear();
    codegen_context->bb_codegen_number = 0;
    codegen_context->functionValues.clear();
    BasicBlock* entryBB = BasicBlock::Create(*codegen_context->TheContext, function->basicBlocks.at(0)->name, TheFunction);
    codegen_context->Builder->SetInsertPoint(entryBB);
    codegen_context->functionBBs.push_back(std::make_pair(function->basicBlocks.at(0).get(), entryBB));
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
        codegen_context->functionVarsAllocas[arg_name] = Alloca;
        j++;
    }
    for (int i = 1; i < function->basicBlocks.size(); i++){
        BasicBlock *BB = BasicBlock::Create(*codegen_context->TheContext, function->basicBlocks.at(i)->name, TheFunction);
        codegen_context->functionBBs.push_back(std::make_pair(function->basicBlocks.at(i).get(), BB));
    }
    int bb_nb = function->basicBlocks.size();
    for (int i = 0; i < bb_nb; i++){
        codegenBasicBlock(codegen_context, fileCIR, std::move(function->basicBlocks.at(i)), bb_nb,  has_sret);
    }


    // TODO : maybe enable this only in somes cases
    std::string error_str;    
    raw_string_ostream string_ostream(error_str);
    if (llvm::verifyFunction(*TheFunction, &string_ostream)){
        std::error_code EC;
        auto dump_file = raw_fd_ostream(StringRef("dump_" + function->proto->name + ".ll"), EC);
        TheFunction->print(dump_file);
        dump_file.write('\n');
        auto function_list = codegen_context->TheModule->functions();
        for (auto f = function_list.begin(), fe = function_list.end(); f != fe; f++){
            if (f->getName() != TheFunction->getName()){
                f->print(dump_file);
            }
        }
        dump_file.close();
        LogErrorV(emptyLoc, "LLVM ERROR : %s\n", error_str.c_str());
        return nullptr;
    }

    // TODO : add memcpy for sret
    return TheFunction;
}

static void codegenGlobalVar(std::unique_ptr<LLVM::Context> &codegen_context, CIR::GlobalVar& global_var, CIR::Function& global_context){
    if (global_var.is_extern && !global_var.Init.is_empty()){
        LogErrorGLLVM("Init Value found even though the global variable is extern"); // TODO
        return;
    }
    GlobalValue::LinkageTypes linkage = GlobalValue::ExternalLinkage;
    Constant* InitVal = nullptr;
    if (!global_var.is_extern){
        if (!global_var.Init.is_empty()){
            InitVal = from_val_to_constant(codegen_context->GlobalValues.at(global_var.Init.get_pos()), global_var.type);
            if (!InitVal){
                LogErrorGLLVM("The constant initialization of the global variable couldn't be converted to a constant");
                return;
            }
        } else {
            InitVal = get_default_constant(*codegen_context->TheContext, global_var.type);
        }
    }
    // TODO : add private global vars
    GlobalVariable* globalVar = new GlobalVariable(*codegen_context->TheModule, get_type_llvm(codegen_context, global_var.type), global_var.is_const, linkage, InitVal, global_var.name);
    if (global_var.section_name != ""){
        globalVar->setSection(global_var.section_name);
    }
    codegen_context->GlobalVars[global_var.name] = globalVar;
}

void codegenFile(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR> fileCIR){
    for (int i = 0; i < fileCIR->strings.size(); i++){
        auto const_str = codegen_context->Builder->CreateGlobalStringPtr(StringRef(fileCIR->strings.at(i)), "", 0, codegen_context->TheModule.get());
        codegen_context->staticStrings.push_back(const_str);
    }
    for (int i = 0; i < fileCIR->global_context.get_instruction_nb(); i++){
        auto instruction = fileCIR->global_context.get_unique_instruction(i);
        codegenInstruction(codegen_context, fileCIR, std::move(instruction), true);
    }
    for (auto& g : fileCIR->global_vars){
        if (g.second != nullptr){
            codegenGlobalVar(codegen_context, *g.second, fileCIR->global_context);
        }
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