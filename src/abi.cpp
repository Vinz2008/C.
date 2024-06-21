#include "abi.h"
#include "codegen.h"
#include "types.h"
#include "llvm/TargetParser/Triple.h"

extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unique_ptr<LLVMContext> TheContext;
extern Triple TripleLLVM;

int get_pointer_size(){ // TODO : remove this (it is not used)
    if (TripleLLVM.isArch32Bit()){
        return 32;
    } else if (TripleLLVM.isArch64Bit()){
        return 64;
    }
    return 64;
}

Value* GetVaAdressSystemV(std::unique_ptr<ExprAST> va){
    if (!dynamic_cast<VariableExprAST*>(va.get())){
        return LogErrorV(emptyLoc, "Not implemented internal function to get address of va list with amd64 systemv abi for non variables");
    }
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    auto varVa = dynamic_cast<VariableExprAST*>(va.get());
    auto vaCodegened = va->codegen();
    return Builder->CreateGEP(vaCodegened->getType(), get_var_allocation(varVa->Name), {zero, zero}, "gep_for_va");
}


bool should_return_struct_with_ptr(Cpoint_Type cpoint_type){
    int max_size = 8; // 64 bit = 8 byte
    if (TripleLLVM.isArch32Bit()){
        max_size = 4;
    } else if (TripleLLVM.isArch64Bit()){
        max_size = 8;
    }
    int size_struct = find_struct_type_size(cpoint_type);
    if (size_struct > max_size){
        return true;
    }
    return false;
}

bool should_pass_struct_byval(Cpoint_Type cpoint_type){
    if (cpoint_type.is_just_struct() && is_struct_all_type(cpoint_type, float_type)){
        int float_nb = struct_get_number_type(cpoint_type, float_type);
        if (float_nb <= 2){
            return false;
        } else {
            return true;
        }
    }
    int max_size = 8; // 64 bit = 8 byte
    if (TripleLLVM.isArch32Bit()){
        max_size = 4; // 32 bit
    } else if (TripleLLVM.isArch64Bit()){
        max_size = 8;
    }
    int size_struct = find_struct_type_size(cpoint_type);
    if (size_struct > max_size){
        return true;
    }
    return false;
}

void addArgSretAttribute(Argument& Arg, Type* type){
    Arg.addAttr(Attribute::getWithStructRetType(*TheContext, type));
    Arg.addAttr(Attribute::getWithAlignment(*TheContext, Align(8)));
}