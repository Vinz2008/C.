//#include "ast.h"
//#include "codegen.h"
#include "llvm/IR/Value.h"

//#include "types.h"

class Cpoint_Type;

class ExprAST;
class StructDeclaration;

using namespace llvm;

Value* GetVaAdressSystemV(std::unique_ptr<ExprAST> va);
bool should_return_struct_with_ptr(Cpoint_Type cpoint_type);
bool should_pass_struct_byval(Cpoint_Type cpoint_type);
void addArgSretAttribute(Argument& Arg, Type* type);
int get_pointer_size();
bool reorder_struct(StructDeclaration* structDeclaration, std::string structName, std::vector<std::pair<std::string,Cpoint_Type>>& return_members);