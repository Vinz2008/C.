#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

namespace operators {
    Value* LLVMCreateAdd(Value* L, Value* R);
    Value* LLVMCreateSub(Value* L, Value* R);
    Value* LLVMCreateMul(Value* L, Value* R);
    Value* LLVMCreateDiv(Value* L, Value* R);
    Value* LLVMCreateRem(Value* L, Value* R);
    Value* LLVMCreateCmp(Value* L, Value* R);
    Value* LLVMCreateNotEqualCmp(Value* L, Value* R);
    Value* LLVMCreateGreaterThan(Value* L, Value* R);
    Value* LLVMCreateGreaterOrEqualThan(Value* L, Value* R);
    Value* LLVMCreateSmallerThan(Value* L, Value* R);
    Value* LLVMCreateSmallerOrEqualThan(Value* L, Value* R);
    Value* LLVMCreateLogicalOr(Value* L, Value* R);
    Value* LLVMCreateLogicalAnd(Value* L, Value* R);
    Value* LLVMCreateAnd(Value* L, Value* R);
}