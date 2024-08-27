#include <memory>
#include "llvm/IR/Value.h"
//#include "ast.h"
using namespace llvm;

class ExprAST;

Value* not_struct_member_call(std::unique_ptr<ExprAST> Expr, std::string functionCall, std::vector<std::unique_ptr<ExprAST>> Args);