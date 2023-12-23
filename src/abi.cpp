#include "abi.h"
#include "codegen.h"
#include "llvm/TargetParser/Triple.h"

extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unique_ptr<LLVMContext> TheContext;

Value* GetVaAdressSystemV(std::unique_ptr<ExprAST> va){
    if (!dynamic_cast<VariableExprAST*>(va.get())){
        return LogErrorV(emptyLoc, "Not implemented internal function to get address of va list with amd64 systemv abi for non variables");
    }
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    auto varVa = dynamic_cast<VariableExprAST*>(va.get());
    auto vaCodegened = va->codegen();
    return Builder->CreateGEP(vaCodegened->getType(), get_var_allocation(varVa->Name), {zero, zero}, "gep_for_va");
}

extern Triple TripleLLVM;

bool should_return_struct_with_ptr(Cpoint_Type cpoint_type){
    int max_size = 64;
    if (TripleLLVM.isArch32Bit()){
        max_size = 32;
    } else if (TripleLLVM.isArch64Bit()){
        max_size = 64;
    }
    int size_struct = find_struct_type_size(cpoint_type);
    if (size_struct > max_size){
        return true;
    }
    return false;
}

bool should_pass_struct_byval(Cpoint_Type cpoint_type){
    return should_return_struct_with_ptr(cpoint_type);
}

void addArgSretAttribute(Argument& Arg, Type* type){
    Arg.addAttr(Attribute::getWithStructRetType(*TheContext, type));
    Arg.addAttr(Attribute::getWithAlignment(*TheContext, Align(8)));
}