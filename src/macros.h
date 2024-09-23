//#include "ast.h"
#include <memory>
#include <vector>

#include "llvm/IR/Value.h"

using namespace llvm;

class StringExprAST;
class ExprAST;
class ArgInlineAsm;


std::unique_ptr<StringExprAST> get_filename_tok();
std::unique_ptr<StringExprAST> stringify_macro(std::unique_ptr<ExprAST>& expr);
std::unique_ptr<ExprAST> generate_expect(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<ExprAST> generate_panic(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<StringExprAST> generate_time_macro();
std::unique_ptr<ExprAST> generate_env_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<StringExprAST> generate_concat_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<ExprAST> generate_asm_macro(std::unique_ptr<StringExprAST> assembly_code, std::vector<ArgInlineAsm> InputOutputArgs);
std::unique_ptr<ExprAST> generate_todo_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<ExprAST> generate_dbg_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<ExprAST> generate_print_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro, bool is_println, bool is_error);
std::unique_ptr<ExprAST> generate_unreachable_macro();
std::unique_ptr<ExprAST> generate_assume_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);

Value* PrintMacroCodegen(std::vector<std::unique_ptr<ExprAST>> Args);
Value* DbgMacroCodegen(std::unique_ptr<ExprAST> VarDbg);
