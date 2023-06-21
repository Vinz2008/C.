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

Value* LLVMCreateRem(Value* L, Value* R){
    if (get_cpoint_type_from_llvm(R->getType())->type == int_type){
      return Builder->CreateSRem(L, R, "remtmp");
    }
    return Builder->CreateFRem(L, R, "fremtmp");
}

}

