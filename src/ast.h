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
#include "types.h"
#include "errors.h"

using namespace llvm;

#ifndef _AST_HEADER_
#define _AST_HEADER_

extern std::unique_ptr<Compiler_context> Comp_context; 

class ExprAST {
public:
  ExprAST(Source_location loc = Comp_context->loc) : loc(loc) {}
  Source_location loc;
  virtual ~ExprAST() = default;
  virtual Value *codegen() = 0;
  int getLine() const { return loc.line_nb; }
  int getCol() const { return loc.col_nb; }
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

class CharExprAST : public ExprAST {
  int c;
public:
  CharExprAST(int c) : c(c) {}
  Value *codegen() override;
};

class BoolExprAST : public ExprAST {
  bool val;
public:
  BoolExprAST(bool val) : val(val) {}
  Value *codegen() override;
};

class AddrExprAST : public ExprAST {
  std::string Name;
public:
  AddrExprAST(const std::string &Name) : Name(Name) {}
  Value *codegen() override;
};

class SizeofExprAST : public ExprAST {
  std::string Name;
public:
  SizeofExprAST(const std::string &Name) : Name(Name) {}
  Value* codegen() override;
};

class VariableExprAST : public ExprAST {
  std::string Name;
  Cpoint_Type type;
public:
  VariableExprAST(const std::string &Name, Cpoint_Type type) : Name(Name), type(type) {}
  Value *codegen() override;
  const std::string &getName() const { return Name; }
};

class StructMemberExprAST : public ExprAST {
  std::string StructName;
  std::string MemberName;
public:
  StructMemberExprAST(const std::string &StructName, const std::string &MemberName) : StructName(StructName), MemberName(MemberName) {}
  Value *codegen() override;
};

/*class ClassMemberExprAST : public ExprAST {
  std::string ClassName;
  std::string MemberName;
public:
  ClassMemberExprAST(const std::string &ClassName, const std::string &MemberName) : ClassName(ClassName), MemberName(MemberName) {}
  Value *codegen() override;
};*/


class ArrayMemberExprAST : public ExprAST {
  std::string ArrayName;
  std::unique_ptr<ExprAST> posAST;
public:
  ArrayMemberExprAST(const std::string &ArrayName, std::unique_ptr<ExprAST> posAST) : ArrayName(ArrayName), posAST(std::move(posAST)) {}
  Value *codegen() override;
};

class BinaryExprAST : public ExprAST {
  std::string Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(const std::string& op, std::unique_ptr<ExprAST> LHS,
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
public:
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
  std::unique_ptr<Cpoint_Type> cpoint_type;
  std::unique_ptr<ExprAST> index;
  bool infer_type;
  //std::unique_ptr<ExprAST> Body;
  VarExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames, std::unique_ptr<Cpoint_Type> cpoint_type, std::unique_ptr<ExprAST> index, bool infer_type = false
             /*,std::unique_ptr<ExprAST> Body*/)
    : VarNames(std::move(VarNames))/*, Body(std::move(Body))*/, cpoint_type(std::move(cpoint_type)), index(std::move(index)), infer_type(infer_type) {}

  Value *codegen() override;
};

class RedeclarationExprAST : public ExprAST {
  std::string VariableName;
  std::unique_ptr<ExprAST> index;
  std::unique_ptr<ExprAST> Val;
  bool has_member;
  std::string member;
public:
  RedeclarationExprAST(const std::string &VariableName, std::unique_ptr<ExprAST> Val, const std::string &member, std::unique_ptr<ExprAST> index = nullptr) 
: VariableName(VariableName), Val(std::move(Val)), member(member), index(std::move(index)) {}
  Value *codegen() override;
};

/*
class StructDeclarAST {
  std::string Name;
  std::vector<std::unique_ptr<VarExprAST>> Vars;
public:
  StructDeclarAST(const std::string &name, std::vector<std::unique_ptr<VarExprAST>> Vars)
    : Name(name), Vars(std::move(Vars)) {}
  Type* codegen();
};

class ClassDeclarAST {
  std::string Name;
  std::vector<std::unique_ptr<FunctionAST>> Functions;
  std::vector<std::unique_ptr<VarExprAST>> Vars;
public:
  ClassDeclarAST(const std::string &name, std::vector<std::unique_ptr<VarExprAST>> Vars, std::vector<std::unique_ptr<FunctionAST>> Functions) 
    : Name(name), Vars(std::move(Vars)), Functions(std::move(Functions)) {}
  Type *codegen();
};*/

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
public:
  bool IsOperator;
  unsigned Precedence;  // Precedence if a binary op.
  Cpoint_Type cpoint_type;
  std::string Name;
  bool is_variable_number_args;
  bool has_template;
  std::string template_name;
  std::vector<std::pair<std::string,Cpoint_Type>> Args;
  PrototypeAST(const std::string &name, std::vector<std::pair<std::string,Cpoint_Type>> Args, Cpoint_Type cpoint_type, bool IsOperator = false, unsigned Prec = 0, bool is_variable_number_args = false, bool has_template = false,  const std::string& template_name = "")
    : Name(name), Args(std::move(Args)), cpoint_type(cpoint_type), IsOperator(IsOperator), Precedence(Prec), is_variable_number_args(is_variable_number_args), has_template(has_template), template_name(template_name) {}

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
public:
  std::unique_ptr<PrototypeAST> Proto;
  std::vector<std::unique_ptr<ExprAST>> Body;
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::vector<std::unique_ptr<ExprAST>> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
  Function *codegen();
};

class StructDeclarAST {
  std::string Name;
  std::vector<std::unique_ptr<FunctionAST>> Functions;
  std::vector<std::unique_ptr<VarExprAST>> Vars;
public:
  StructDeclarAST(const std::string &name, std::vector<std::unique_ptr<VarExprAST>> Vars, std::vector<std::unique_ptr<FunctionAST>> Functions) 
    : Name(name), Vars(std::move(Vars)), Functions(std::move(Functions)) {}
  Type *codegen();
};



class GlobalVariableAST {
public:
  std::string varName;
  bool is_const;
  std::unique_ptr<Cpoint_Type> cpoint_type;
  std::unique_ptr<ExprAST> Init;
  GlobalVariableAST(const std::string& varName, bool is_const, std::unique_ptr<Cpoint_Type> cpoint_type, std::unique_ptr<ExprAST> Init) 
    : varName(varName), is_const(is_const), cpoint_type(std::move(cpoint_type)), Init(std::move(Init)) {} 
  GlobalVariable* codegen();
};

class TypeDefAST {
public:
  std::string new_type;
  std::string value_type;
  TypeDefAST(const std::string& new_type, const std::string& value_type) :  new_type(new_type), value_type(value_type) {}
  void codegen();
};

class ModAST {
public:
  std::string mod_name;
  ModAST(const std::string& mod_name) :  mod_name(mod_name) {}
  void codegen();
};

class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond;
  std::vector<std::unique_ptr<ExprAST>> Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Then,
            std::vector<std::unique_ptr<ExprAST>> Else)
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

class BreakExprAST : public ExprAST {
public:
  BreakExprAST() {}
  Value* codegen() override;
};

class GotoExprAST : public ExprAST {
  std::string label_name;
public:
  GotoExprAST(const std::string& label_name) : label_name(label_name) {}
  Value* codegen() override;
};

class LabelExprAST : public ExprAST {
  std::string label_name;
public:
  LabelExprAST(const std::string& label_name) : label_name(label_name) {}
  Value* codegen() override;
};

class ForExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Start, End, Step;
  std::vector<std::unique_ptr<ExprAST>> Body;

public:
  ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::vector<std::unique_ptr<ExprAST>> Body)
    : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
      Step(std::move(Step)), Body(std::move(Body)) {}

  Value *codegen() override;
};

class WhileExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond;
  std::vector<std::unique_ptr<ExprAST>> Body;
public:
  WhileExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Body) : Cond(std::move(Cond)), Body(std::move(Body)) {}
  Value *codegen() override;
};

class LoopExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Array;
  std::vector<std::unique_ptr<ExprAST>> Body;
  bool is_infinite_loop;
public:
  LoopExprAST(std::string VarName, std::unique_ptr<ExprAST> Array, std::vector<std::unique_ptr<ExprAST>> Body, bool is_infinite_loop = false) : VarName(VarName), Array(std::move(Array)), Body(std::move(Body)), is_infinite_loop(is_infinite_loop) {}
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
std::unique_ptr<StructDeclarAST> ParseStruct();
//std::unique_ptr<ClassDeclarAST> ParseClass();
std::unique_ptr<ExprAST> ParseAddrExpr();
std::unique_ptr<ExprAST> ParseSizeofExpr();
std::unique_ptr<ExprAST> ParseArrayMemberExpr();
std::unique_ptr<ExprAST> ParseWhileExpr();
std::unique_ptr<ExprAST> ParseGotoExpr();
std::unique_ptr<ExprAST> ParseLabelExpr();
std::unique_ptr<GlobalVariableAST> ParseGlobalVariable();
std::unique_ptr<TypeDefAST> ParseTypeDef();
std::unique_ptr<ExprAST> ParseBool(bool bool_value);
std::unique_ptr<ExprAST> ParseCharExpr();
std::unique_ptr<ExprAST> ParseLoopExpr();
std::unique_ptr<ExprAST> ParseBreakExpr();
std::unique_ptr<ModAST> ParseMod();

std::unique_ptr<ExprAST> vLogError(const char* Str, va_list args);
std::unique_ptr<ExprAST> LogError(const char *Str, ...);

#endif
