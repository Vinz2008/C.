#include <memory>
#include "ast.h"

enum Token {
  tok_eof = -1,

  // commands
  tok_func = -2,
  tok_extern = -3,

  // primary
  tok_identifier = -4,
  tok_number = -5,
  tok_if = -6,
  tok_then = -7,
  tok_else = -8,
  tok_for = -9,
  tok_in = -10,

  // operators
  tok_binary = -11,
  tok_unary = -12,

  // var definition
  tok_var = -13,
  tok_return = -14,
  tok_string = -15,
  tok_object = -16,
  tok_addr = -17,
  tok_ptr = -19,
  tok_return_arrow = -20,
};

int getNextToken();
int GetTokPrecedence();
std::unique_ptr<ExprAST> LogError(const char *Str);
void go_to_next_line();
std::string get_line_returned();