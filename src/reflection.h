//#include "llvm/IR/LLVMContext.h"
//#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
//#include "llvm/IR/Type.h"
//#include "ast.h"

using namespace llvm;

class ExprAST;

class Cpoint_Type;

int getTypeId(Cpoint_Type cpoint_type);
Value* getTypeIdLLVM(Cpoint_Type cpoint_type);
Value* refletionInstruction(std::string instruction, std::vector<std::unique_ptr<ExprAST>> Args);