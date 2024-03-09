#include "members.h"

#include <memory>


// implement members for ints and other built-in types
// TODO 

Value* number_member_call(std::unique_ptr<NumberExprAST> numberExpr){
    // temporary test value
    return ConstantInt::get(get_type_llvm(int_type), APInt(32, 144, true));
}