#include "members.h"
#include <memory>
#include "types.h"
#include "log.h"
#include "ast.h"

Value* not_struct_member_call(std::unique_ptr<ExprAST> Expr, std::string functionCall, std::vector<std::unique_ptr<ExprAST>> Args){
    Cpoint_Type expr_type = /*get_cpoint_type_from_llvm(Expr->clone()->codegen()->getType())*/ Expr->get_type();
    Log::Info() << "expr_type : " << expr_type << "\n";
    std::string mangled_function_name = expr_type.create_mangled_name() + "__" + functionCall;
    Args.insert(Args.begin(), std::move(Expr));
    return std::make_unique<CallExprAST>(emptyLoc, mangled_function_name, std::move(Args), Cpoint_Type())->codegen();
}