#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"
#include "types.h"

using namespace llvm;

namespace operators {
    Value* LLVMCreateAdd(Value* L, Value* R, Cpoint_Type arg_type);
    Value* LLVMCreateSub(Value* L, Value* R, Cpoint_Type arg_type);
    Value* LLVMCreateMul(Value* L, Value* R, Cpoint_Type arg_type);
    Value* LLVMCreateDiv(Value* L, Value* R, Cpoint_Type type);
    Value* LLVMCreateRem(Value* L, Value* R/*, Cpoint_Type type*/);
    Value* LLVMCreateCmp(Value* L, Value* R, Cpoint_Type arg_type);
    Value* LLVMCreateNotEqualCmp(Value* L, Value* R, Cpoint_Type arg_type);
    Value* LLVMCreateGreaterThan(Value* L, Value* R, Cpoint_Type arg_type);
    Value* LLVMCreateGreaterOrEqualThan(Value* L, Value* R, Cpoint_Type arg_type);
    Value* LLVMCreateSmallerThan(Value* L, Value* R, Cpoint_Type arg_type);
    Value* LLVMCreateSmallerOrEqualThan(Value* L, Value* R, Cpoint_Type arg_type);
    Value* LLVMCreateLogicalOr(Value* L, Value* R);
    Value* LLVMCreateLogicalAnd(Value* L, Value* R);
    Value* LLVMCreateAnd(Value* L, Value* R);
    void installPrecendenceOperators();
}