#include "members.h"
#include "types.h"
#include <memory>


Value* not_struct_member_call(std::unique_ptr<ExprAST> Expr, std::string functionCall, std::vector<std::unique_ptr<ExprAST>> Args){
    Cpoint_Type expr_type = get_cpoint_type_from_llvm(Expr->codegen()->getType());
    std::string mangled_function_name = create_mangled_name_from_type(expr_type) + "__" + functionCall;
    Args.insert(Args.begin(), std::make_unique<LLVMASTValueWrapper>(Expr->codegen()));
    return std::make_unique<CallExprAST>(emptyLoc, mangled_function_name, std::move(Args), Cpoint_Type())->codegen();
}