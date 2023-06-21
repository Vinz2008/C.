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
    if (get_cpoint_type_from_llvm(R->getType())->type == int_type){
      return Builder->CreateAdd(L, R, "addtmp");
    }
    return Builder->CreateFAdd(L, R, "faddtmp");
}

Value* LLVMCreateSub(Value* L, Value* R){
    if (get_cpoint_type_from_llvm(R->getType())->type == int_type){
      return Builder->CreateSub(L, R, "subtmp");
    }
    return Builder->CreateFSub(L, R, "fsubtmp");
}

Value* LLVMCreateMul(Value* L, Value* R){
    if (get_cpoint_type_from_llvm(R->getType())->type == int_type){
      return Builder->CreateMul(L, R, "multmp");
    }
    return Builder->CreateFMul(L, R, "fmultmp");
}

Value* LLVMCreateDiv(Value* L, Value* R){
    if (get_cpoint_type_from_llvm(R->getType())->type == int_type){
      return Builder->CreateSDiv(L, R, "divtmp");
    }
    return Builder->CreateFDiv(L, R, "fdivtmp");
}

Value* LLVMCreateRem(Value* L, Value* R){
    if (get_cpoint_type_from_llvm(R->getType())->type == int_type){
      return Builder->CreateSRem(L, R, "remtmp");
    }
    return Builder->CreateFRem(L, R, "fremtmp");
}

Value* LLVMCreateCmp(Value* L, Value* R){
    if (get_cpoint_type_from_llvm(R->getType())->type == int_type){
      L = Builder->CreateICmpEQ(L, R, "cmptmp");
    } else {
      L = Builder->CreateFCmpUEQ(L, R, "cmptmp");
    }
    return Builder->CreateUIToFP(L, get_type_llvm(Cpoint_Type(double_type)), "booltmp");
}

Value* LLVMCreateNotEqualCmp(Value* L, Value* R){
    if (get_cpoint_type_from_llvm(R->getType())->type == int_type){
        L = Builder->CreateICmpNE(L, R, "notequalcmptmp");
    } else {
        L = Builder->CreateFCmpUNE(L, R, "notequalfcmptmp");
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

