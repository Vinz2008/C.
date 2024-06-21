#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include "ast.h"

using namespace llvm;

Value* getTypeId(Value* valueLLVM);
Value* refletionInstrTypeId(std::vector<std::unique_ptr<ExprAST>> Args);
Value* refletionInstrGetStructName(std::vector<std::unique_ptr<ExprAST>> Args);
Value* refletionInstrGetMemberNb(std::vector<std::unique_ptr<ExprAST>> Args);
Value* refletionInstruction(std::string instruction, std::vector<std::unique_ptr<ExprAST>> Args);