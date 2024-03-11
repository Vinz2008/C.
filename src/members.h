#include <memory>
#include "ast.h"

Value* not_struct_member_call(std::unique_ptr<ExprAST> Expr, std::string functionCall, std::vector<std::unique_ptr<ExprAST>> Args);