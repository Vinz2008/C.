#include "operators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DIBuilder.h"
#include "types.h"

using namespace llvm;
extern std::unique_ptr<IRBuilder<>> Builder;

namespace operators {

Value* LLVMCreateAdd(Value* L, Value* R){
    if (!is_decimal_number_type(get_cpoint_type_from_llvm(R->getType()))){
      return Builder->CreateAdd(L, R, "addtmp");
    }
    return Builder->CreateFAdd(L, R, "faddtmp");
}

Value* LLVMCreateSub(Value* L, Value* R){
    if (!is_decimal_number_type(get_cpoint_type_from_llvm(R->getType()))){
      return Builder->CreateSub(L, R, "subtmp");
    }
    return Builder->CreateFSub(L, R, "fsubtmp");
}

Value* LLVMCreateMul(Value* L, Value* R){
    if (!is_decimal_number_type(get_cpoint_type_from_llvm(R->getType()))){
      return Builder->CreateMul(L, R, "multmp");
    }
    return Builder->CreateFMul(L, R, "fmultmp");
}

Value* LLVMCreateDiv(Value* L, Value* R){
    Cpoint_Type type = get_cpoint_type_from_llvm(R->getType());
    if (!is_decimal_number_type(type)){
      if (is_signed(type)){
        return Builder->CreateSDiv(L, R, "sdivtmp");
      } else {
        return Builder->CreateUDiv(L, R, "udivtmp");
      }
    }
    return Builder->CreateFDiv(L, R, "fdivtmp");
}

Value* LLVMCreateRem(Value* L, Value* R){
    Cpoint_Type type = get_cpoint_type_from_llvm(R->getType());
    if (!is_decimal_number_type(type)){
      if (is_signed(type)){
        return Builder->CreateSRem(L, R, "sremtmp");
      } else {
        return Builder->CreateURem(L, R, "uremtmp");
      }
    }
    return Builder->CreateFRem(L, R, "fremtmp");
}

Value* LLVMCreateCmp(Value* L, Value* R){
    if (!is_decimal_number_type(get_cpoint_type_from_llvm(R->getType()))){
      L = Builder->CreateICmpEQ(L, R, "cmptmp");
    } else {
      L = Builder->CreateFCmpUEQ(L, R, "cmptmp");
    }
    return Builder->CreateUIToFP(L, get_type_llvm(Cpoint_Type(double_type)), "booltmp");
}

Value* LLVMCreateNotEqualCmp(Value* L, Value* R){
    if (!is_decimal_number_type(get_cpoint_type_from_llvm(R->getType()))){
        L = Builder->CreateICmpNE(L, R, "notequalcmptmp");
    } else {
        L = Builder->CreateFCmpUNE(L, R, "notequalfcmptmp");
    }
    return Builder->CreateUIToFP(L, get_type_llvm(Cpoint_Type(double_type)), "booltmp");
}

Value* LLVMCreateGreaterThan(Value* L, Value* R){
    if (!is_decimal_number_type(get_cpoint_type_from_llvm(R->getType()))){
      L = Builder->CreateICmpSGT(L, R, "cmptmp");
    } else {
      L = Builder->CreateFCmpOGT(L, R, "cmptmp");
    }
    return Builder->CreateUIToFP(L, get_type_llvm(Cpoint_Type(double_type)), "booltmp");
}

Value* LLVMCreateGreaterOrEqualThan(Value* L, Value* R){
    if (!is_decimal_number_type(get_cpoint_type_from_llvm(R->getType()))){
        L = Builder->CreateICmpSGE(L, R, "cmptmp");
    } else {
        L = Builder->CreateFCmpOGE(L, R, "cmptmp");
    }
    return Builder->CreateUIToFP(L, get_type_llvm(Cpoint_Type(double_type)), "booltmp");
}

Value* LLVMCreateSmallerThan(Value* L, Value* R){
    if (!is_decimal_number_type(get_cpoint_type_from_llvm(R->getType()))){
      L = Builder->CreateICmpSLT(L, R, "cmptmp");
    } else {
      L = Builder->CreateFCmpOLT(L, R, "cmptmp");
    }
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder->CreateUIToFP(L, get_type_llvm(Cpoint_Type(double_type)), "booltmp");
}

Value* LLVMCreateSmallerOrEqualThan(Value* L, Value* R){
    if (!is_decimal_number_type(get_cpoint_type_from_llvm(R->getType()))){
        L = Builder->CreateICmpSLE(L, R, "cmptmp");
    } else {
        L = Builder->CreateFCmpOLE(L, R, "cmptmp");
    }
    return Builder->CreateUIToFP(L, get_type_llvm(Cpoint_Type(double_type)), "booltmp");
}

Value* LLVMCreateLogicalOr(Value* L, Value* R){
    L = Builder->CreateOr(L, R, "logicalortmp");
    return Builder->CreateUIToFP(L, get_type_llvm(Cpoint_Type(double_type)), "booltmp");
}

Value* LLVMCreateLogicalAnd(Value* L, Value* R){
    L = Builder->CreateAnd(L, R, "logicalandtmp");
    return Builder->CreateUIToFP(L, get_type_llvm(Cpoint_Type(double_type)), "booltmp");
}



}

