#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

namespace operators {
    Value* LLVMCreateAdd(Value* L, Value* R);
    Value* LLVMCreateSub(Value* L, Value* R);
    Value* LLVMCreateMul(Value* L, Value* R);
    Value* LLVMCreateRem(Value* L, Value* R);
}