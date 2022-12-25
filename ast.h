#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <iostream>
#include <memory>
#include <vector>

using namespace llvm;

#ifndef _AST_HEADER_
#define _AST_HEADER_

class ExprAST {
public:
  virtual ~ExprAST() = default;
  virtual Value *codegen() = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val) : Val(Val) {}
  Value *codegen() override;
};

class StringExprAST : public ExprAST {
  std::string str;
public:
  StringExprAST(const std::string &str) : str(str) {}
  Value *codegen() override;
};

class VariableExprAST : public ExprAST {
  std::string Name;
  int type;
public:
  VariableExprAST(const std::string &Name, int type) : Name(Name), type(type) {}
  Value *codegen() override;
  const std::string &getName() const { return Name; }
};

class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
    : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  Value *codegen() override;
};

class UnaryExprAST : public ExprAST {
  char Opcode;
  std::unique_ptr<ExprAST> Operand;

public:
  UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
    : Opcode(Opcode), Operand(std::move(Operand)) {}

  Value *codegen() override;
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
      : Callee(Callee), Args(std::move(Args)) {}
  Value *codegen() override;
};

class VarExprAST : public ExprAST {
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
  //std::unique_ptr<ExprAST> Body;

public:
  VarExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames
             /*,std::unique_ptr<ExprAST> Body*/)
    : VarNames(std::move(VarNames))/*, Body(std::move(Body))*/ {}

  Value *codegen() override;
};

class ObjectDeclarAST {
  std::string Name;
  std::vector<std::unique_ptr<VarExprAST>> Vars;
public:
  ObjectDeclarAST(const std::string &name, std::vector<std::unique_ptr<VarExprAST>> Vars)
    : Name(name), Vars(std::move(Vars)) {}
  Value* codegen();
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
  bool IsOperator;
  unsigned Precedence;  // Precedence if a binary op.

public:
  PrototypeAST(const std::string &name, std::vector<std::string> Args, bool IsOperator = false, unsigned Prec = 0)
    : Name(name), Args(std::move(Args)), IsOperator(IsOperator), Precedence(Prec) {}

  const std::string &getName() const { return Name; }
  Function *codegen();
  bool isUnaryOp() const { return IsOperator && Args.size() == 1; }
  bool isBinaryOp() const { return IsOperator && Args.size() == 2; }
  char getOperatorName() const {
    assert(isUnaryOp() || isBinaryOp());
    return Name[Name.size() - 1];
  }
  unsigned getBinaryPrecedence() const { return Precedence; }
};

class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
  Function *codegen();
};

class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond, Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
            std::unique_ptr<ExprAST> Else)
    : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

  Value *codegen() override;
};

class ReturnAST : public ExprAST {
  std::unique_ptr<ExprAST> returned_expr;
  //double Val;
public:
  ReturnAST(std::unique_ptr<ExprAST> returned_expr)
  : returned_expr(std::move(returned_expr)) {}
  //ReturnAST(double val) : Val(val) {}
  Value *codegen() override;

};

class ForExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Start, End, Step, Body;

public:
  ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::unique_ptr<ExprAST> Body)
    : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
      Step(std::move(Step)), Body(std::move(Body)) {}

  Value *codegen() override;
};


std::unique_ptr<ExprAST> ParseExpression();
std::unique_ptr<ExprAST> ParsePrimary();
std::unique_ptr<FunctionAST> ParseDefinition();
std::unique_ptr<PrototypeAST> ParseExtern();
std::unique_ptr<FunctionAST> ParseTopLevelExpr();
std::unique_ptr<ExprAST> ParseIfExpr();
std::unique_ptr<ExprAST> ParseReturn();
std::unique_ptr<ExprAST> ParseForExpr();
std::unique_ptr<ExprAST> ParseStrExpr();
std::unique_ptr<ExprAST> ParseUnary();
std::unique_ptr<ExprAST> ParseVarExpr();


#endif
