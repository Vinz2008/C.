#include "ast.h"

Value* GetVaAdressSystemV(std::unique_ptr<ExprAST> va);
bool should_return_struct_with_ptr(Cpoint_Type cpoint_type);
bool should_pass_struct_byval(Cpoint_Type cpoint_type);
void addArgSretAttribute(Argument& Arg, Type* type);
int get_pointer_size();