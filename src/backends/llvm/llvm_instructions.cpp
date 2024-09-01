#include "llvm_instructions.h"
#include "llvm.h"
#include "../../CIR/cir.h"
#include "../../abi.h"
#include "../../log.h"
#include "llvm/IR/Function.h"

using namespace llvm;

namespace LLVM {

static void codegenInstruction() {
    
}

static Function* codegenProto(std::unique_ptr<LLVM::Context>& codegen_context, CIR::FunctionProto proto){
    // TODO!
    std::vector<Type *> type_args;
    for (int i = 0; i < proto.args.size(); i++){
        Cpoint_Type arg_type = proto.args.at(i).second;
        if (arg_type.is_just_struct() && is_struct_all_type(arg_type, float_type) && struct_get_number_type(arg_type, float_type) <= 2) {
            type_args.push_back(vector_type_from_struct(arg_type));
        } else {
            if (arg_type.is_just_struct() && should_pass_struct_byval(arg_type)){
                arg_type.is_ptr = true;
            }
            type_args.push_back(get_type_llvm(arg_type));
        }
    }
    bool has_sret = false;
    // TODO : add main type (probably already added)
    if (proto.return_type.is_just_struct()){
        if (should_return_struct_with_ptr(proto.return_type)){
            has_sret = true;
            Cpoint_Type sret_arg_type = proto.return_type;
            sret_arg_type.is_ptr = true;
            type_args.insert(type_args.begin(), get_type_llvm(sret_arg_type));
        }
    }
    Cpoint_Type return_type = (has_sret) ? Cpoint_Type(void_type) : proto.return_type;
    FunctionType* FT = FunctionType::get(get_type_llvm(return_type), type_args, proto.is_variable_number_args);
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
            addArgSretAttribute(Arg, get_type_llvm(proto.return_type));
            Arg.setName("sret_arg");
            Idx = 0;
            has_added_sret = true;
        } else if (proto.args.at(Idx).second.is_just_struct() && should_pass_struct_byval(proto.args.at(Idx).second)){
            Log::Info() << "should_pass_struct_byval arg " << proto.args.at(Idx).first << " : " << proto.args.at(Idx).second << "\n";
            Cpoint_Type arg_type = get_cpoint_type_from_llvm(get_type_llvm(proto.args.at(Idx).second));
            Cpoint_Type by_val_ptr_type = arg_type;
            by_val_ptr_type.is_ptr = true;
            by_val_ptr_type.nb_ptr++;
            Arg.mutateType(get_type_llvm(by_val_ptr_type));
            Arg.addAttr(Attribute::getWithByValType(*codegen_context->TheContext, get_type_llvm(arg_type)));
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
static void codegenBasicBlock(std::unique_ptr<LLVM::Context>& codegen_context, Function* TheFunction, std::unique_ptr<CIR::BasicBlock> basic_block){
    BasicBlock *BB = BasicBlock::Create(*codegen_context->TheContext, basic_block->name, TheFunction);
    codegen_context->Builder->SetInsertPoint(BB);
}

static Function* codegenFunction(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR> &fileCIR, std::unique_ptr<CIR::Function> function) {
    Function *TheFunction = getFunction(codegen_context, fileCIR, function->proto->name);
    // TODO : uncomment this after implementing codegen of instructions
    /*for (int i = 0; i < function->basicBlocks.size(); i++){
        codegenBasicBlock(codegen_context, TheFunction, std::move(function->basicBlocks.at(i)));
    }*/
    return TheFunction;
}

void codegenFile(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR> fileCIR){
    for (int i = 0; i < fileCIR->functions.size(); i++){
        codegenFunction(codegen_context, fileCIR, std::move(fileCIR->functions.at(i)));
    }
}

}