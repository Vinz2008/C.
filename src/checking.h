#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "types.h"

using namespace llvm;

Value* bound_checking_constant_index_array_member(Constant* indexConst, Cpoint_Type cpoint_type, Source_location loc);