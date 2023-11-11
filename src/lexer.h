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
  tok_struct = -16,
  tok_addr = -17,
  tok_class = -18,
  tok_ptr = -19,
  tok_import = -20,
  tok_while = -21,
  tok_op_multi_char = -22,
  tok_label = -23,
  tok_goto = -24,
  tok_true = -25,
  tok_false = -26,
  tok_char = -27,
  tok_sizeof = -28,
  tok_format = -29,
  tok_loop = -30,
  tok_typedef = -31,
  tok_break = -32,
  tok_mod = -33,
  tok_single_line_comment = -34,
  tok_typeid = -35,
  tok_null = -36,
  tok_cast = -37,
  tok_union = -38,
  tok_enum = -39,
  tok_match = -40,
  tok_members = -41,
  tok_deref = -42,
};

int getNextToken();
int GetTokPrecedence();
int getTokPrecedenceMultiChar(std::string op);
void go_to_next_line();
std::string get_line_returned();
void handlePreprocessor();