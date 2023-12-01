#include "ast.h"

std::unique_ptr<StringExprAST> get_filename_tok();
std::unique_ptr<StringExprAST> stringify_macro(std::unique_ptr<ExprAST> expr);
std::unique_ptr<ExprAST> generate_expect(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<ExprAST> generate_panic(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<StringExprAST> generate_time_macro();
std::unique_ptr<ExprAST> generate_env_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<StringExprAST> generate_concat_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<ExprAST> generate_asm_macro(/*std::vector<std::unique_ptr<ExprAST>>& ArgsMacro*/ std::unique_ptr<ArgsInlineAsm> ArgsMacro);
std::unique_ptr<ExprAST> generate_todo_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<ExprAST> generate_dbg_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro);
std::unique_ptr<ExprAST> generate_print_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro, bool is_println);