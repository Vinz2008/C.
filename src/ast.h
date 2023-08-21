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
#include <utility>

using namespace llvm;

#ifndef _AST_HEADER_
#define _AST_HEADER_

#include "types.h"
#include "errors.h"
#include "log.h"

extern std::unique_ptr<Compiler_context> Comp_context; 

class StructDeclarAST;

//std::unique_ptr<StructDeclarAST> LogErrorS(const char *Str, ...);

class ExprAST {
public:
  ExprAST(Source_location loc = Comp_context->lexloc) : loc(loc) {}
  Source_location loc;
  virtual ~ExprAST() = default;
  virtual Value *codegen() = 0;
  virtual std::unique_ptr<ExprAST> clone() = 0;
  int getLine() const { return loc.line_nb; }
  int getCol() const { return loc.col_nb; }
};

class EmptyExprAST : public ExprAST {
public:
    EmptyExprAST(){};
    Value *codegen() override { return nullptr; }
    std::unique_ptr<ExprAST> clone() override { return std::make_unique<EmptyExprAST>(); }

};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val) : Val(Val) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<NumberExprAST>(Val);
  }
};

class StringExprAST : public ExprAST {
  std::string str;
public:
  StringExprAST(const std::string &str) : str(str) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<StringExprAST>(str);
  }
};

class CharExprAST : public ExprAST {
  int c;
public:
  CharExprAST(int c) : c(c) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<CharExprAST>(c);
  }
};

class BoolExprAST : public ExprAST {
  bool val;
public:
  BoolExprAST(bool val) : val(val) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<BoolExprAST>(val);
  }
};

class AddrExprAST : public ExprAST {
  std::string Name;
public:
  AddrExprAST(const std::string &Name) : Name(Name) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<AddrExprAST>(Name);
  }
};

class SizeofExprAST : public ExprAST {
  Cpoint_Type type;
  bool is_variable;
  std::string Name;
public:
  SizeofExprAST(Cpoint_Type type, bool is_variable, const std::string &Name) : type(type), is_variable(is_variable), Name(Name) {}
  Value* codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<SizeofExprAST>(type,is_variable, Name);
  }
};

class TypeidExprAST : public ExprAST {
    std::unique_ptr<ExprAST> val;
public:
    TypeidExprAST(std::unique_ptr<ExprAST> val) : val(std::move(val)) {}
    Value* codegen() override;
    std::unique_ptr<ExprAST> clone() override {
        return std::make_unique<TypeidExprAST>(val->clone());
    }
};

class VariableExprAST : public ExprAST {
  std::string Name;
  Cpoint_Type type;
public:
  VariableExprAST(Source_location Loc, const std::string &Name, Cpoint_Type type) : ExprAST(Loc), Name(Name), type(type) {}
  Value *codegen() override;
  const std::string &getName() const { return Name; }
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<VariableExprAST>(ExprAST::loc, Name, type);
  }
};

class StructMemberExprAST : public ExprAST {
  std::string StructName;
  std::string MemberName;
  bool is_function_call;
  std::vector<std::unique_ptr<ExprAST>> Args;
public:
  StructMemberExprAST(const std::string &StructName, const std::string &MemberName, bool is_function_call, std::vector<std::unique_ptr<ExprAST>> Args) : StructName(StructName), MemberName(MemberName), is_function_call(is_function_call), Args(std::move(Args)) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    std::vector<std::unique_ptr<ExprAST>> ArgsCloned;
    if (!ArgsCloned.empty()){
      for (int i = 0; i < ArgsCloned.size(); i++){
        ArgsCloned.push_back(Args.at(i)->clone());
      }
    }
    return std::make_unique<StructMemberExprAST>(StructName, MemberName, is_function_call, std::move(ArgsCloned));
  }
};

/*class ClassMemberExprAST : public ExprAST {
  std::string ClassName;
  std::string MemberName;
public:
  ClassMemberExprAST(const std::string &ClassName, const std::string &MemberName) : ClassName(ClassName), MemberName(MemberName) {}
  Value *codegen() override;
};*/

class UnionMemberExprAST : public ExprAST {
public:
  std::string UnionName;
  std::string MemberName;
  UnionMemberExprAST(const std::string &UnionName, const std::string &MemberName) : UnionName(UnionName), MemberName(MemberName) {}
  Value* codegen();
  std::unique_ptr<ExprAST> clone(){
    return std::make_unique<UnionMemberExprAST>(UnionName, MemberName);
  }
};

class ConstantArrayExprAST : public ExprAST {
public:
  std::vector<std::unique_ptr<ExprAST>> ArrayMembers;
  ConstantArrayExprAST(std::vector<std::unique_ptr<ExprAST>> ArrayMembers) : ArrayMembers(std::move(ArrayMembers)) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    std::vector<std::unique_ptr<ExprAST>> ArrayMembersClone;
    if (!ArrayMembers.empty()){
      for (int i = 0; i < ArrayMembers.size(); i++){
        ArrayMembersClone.push_back(ArrayMembers.at(i)->clone());
      }
    }
    return std::make_unique<ConstantArrayExprAST>(std::move(ArrayMembersClone));
   }
};

class EnumCreation : public ExprAST {
public:
    std::string EnumVarName;
    std::string EnumMemberName;
    std::unique_ptr<ExprAST> value;
    EnumCreation(const std::string& EnumVarName, const std::string& EnumMemberName, std::unique_ptr<ExprAST> value) : EnumVarName(EnumVarName), EnumMemberName(EnumMemberName), value(std::move(value)) {}
    llvm::Value* codegen() override;
    //Value *codegen() override;
    std::unique_ptr<ExprAST> clone() override {
        std::unique_ptr<ExprAST> valueCloned = nullptr;
        if (value){
            valueCloned = value->clone();
        }
        return std::make_unique<EnumCreation>(EnumVarName, EnumMemberName, std::move(valueCloned));
    }
};


class ArrayMemberExprAST : public ExprAST {
  std::string ArrayName;
  std::unique_ptr<ExprAST> posAST;
public:
  ArrayMemberExprAST(const std::string &ArrayName, std::unique_ptr<ExprAST> posAST) : ArrayName(ArrayName), posAST(std::move(posAST)) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<ArrayMemberExprAST>(ArrayName, posAST->clone());
  }
};

class BinaryExprAST : public ExprAST {
  std::string Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(Source_location Loc, const std::string& op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
    : ExprAST(Loc), Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<BinaryExprAST>(ExprAST::loc, Op, LHS->clone(), RHS->clone());
  }
};

class UnaryExprAST : public ExprAST {
  char Opcode;
  std::unique_ptr<ExprAST> Operand;

public:
  UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
    : Opcode(Opcode), Operand(std::move(Operand)) {}

  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<UnaryExprAST>(Opcode, Operand->clone());
  }
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;
  /*std::string*/ Cpoint_Type template_passed_type;

public:
  CallExprAST(Source_location Loc, const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args, /*const std::string&*/ Cpoint_Type template_passed_type)
      : ExprAST(Loc), Callee(Callee), Args(std::move(Args)), template_passed_type(template_passed_type) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    std::vector<std::unique_ptr<ExprAST>> ArgsCloned;
    if (!Args.empty()){
      for (int i = 0; i < Args.size(); i++){
        ArgsCloned.push_back(Args.at(i)->clone());
      }
    }
    return std::make_unique<CallExprAST>(ExprAST::loc, Callee, std::move(ArgsCloned), template_passed_type);
  }
};

class VarExprAST : public ExprAST {
public:
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
  Cpoint_Type cpoint_type;
  std::unique_ptr<ExprAST> index;
  bool infer_type;
  //std::unique_ptr<ExprAST> Body;
  VarExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames, Cpoint_Type cpoint_type, std::unique_ptr<ExprAST> index, bool infer_type = false
             /*,std::unique_ptr<ExprAST> Body*/)
    : VarNames(std::move(VarNames))/*, Body(std::move(Body))*/, cpoint_type(cpoint_type), index(std::move(index)), infer_type(infer_type) {}

  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNamesCloned;
    if (!VarNames.empty()){
      Log::Info() << "VarNames.size() " << VarNames.size() << "\n";
      for (int i = 0; i < VarNames.size(); i++){
        std::string name = VarNames.at(i).first;
        if (VarNames.at(i).second == nullptr){
          Log::Info() << "VarNames.at(i).second is nullptr" << "\n";
        }
        std::unique_ptr<ExprAST> val;
        if (VarNames.at(i).second != nullptr){
        val = VarNames.at(i).second->clone();
        } else {
          val = nullptr;
        }
        VarNamesCloned.push_back(std::make_pair(name, std::move(val)));
      }
    }
    std::unique_ptr<ExprAST> indexCloned;
    if (index == nullptr){
      indexCloned = nullptr;
    } else {
      indexCloned = index->clone();
    }
    return std::make_unique<VarExprAST>(std::move(VarNamesCloned), cpoint_type, std::move(indexCloned), infer_type);
  }
};

class RedeclarationExprAST : public ExprAST {
  std::string VariableName;
  std::unique_ptr<ExprAST> Val;
  std::string member;
  std::unique_ptr<ExprAST> index;
public:
  RedeclarationExprAST(const std::string &VariableName, std::unique_ptr<ExprAST> Val, const std::string &member, std::unique_ptr<ExprAST> index = nullptr) 
: VariableName(VariableName), Val(std::move(Val)), member(member), index(std::move(index)) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    std::unique_ptr<ExprAST> indexCloned = nullptr;
    if (index != nullptr){
      indexCloned = index->clone();
    }
    return std::make_unique<RedeclarationExprAST>(VariableName, Val->clone(), member, std::move(indexCloned));
  }
};

class CastExprAST : public ExprAST {
public:
    std::unique_ptr<ExprAST> ValToCast;
    Cpoint_Type type;
    CastExprAST(std::unique_ptr<ExprAST> ValToCast, Cpoint_Type type) : ValToCast(std::move(ValToCast)), type(type) {}
    Value *codegen() override;
    std::unique_ptr<ExprAST> clone(){
        return std::make_unique<CastExprAST>(ValToCast->clone(), type);
    }
};

class NullExprAST : public ExprAST {
public:
    NullExprAST() {}
    Value *codegen() override;
    std::unique_ptr<ExprAST> clone(){
        return std::make_unique<NullExprAST>();
    }
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
  int Line;
  PrototypeAST(Source_location loc, const std::string &name, std::vector<std::pair<std::string,Cpoint_Type>> Args, Cpoint_Type cpoint_type, bool IsOperator = false, unsigned Prec = 0, bool is_variable_number_args = false, bool has_template = false,  const std::string& template_name = "")
    : Name(name), Args(std::move(Args)), cpoint_type(cpoint_type), IsOperator(IsOperator), Precedence(Prec), is_variable_number_args(is_variable_number_args), has_template(has_template), template_name(template_name), Line(loc.line_nb) {}

  const std::string &getName() const { return Name; }
  Function *codegen();
  bool isUnaryOp() const { return IsOperator && Args.size() == 1; }
  bool isBinaryOp() const { return IsOperator && Args.size() == 2; }
  char getOperatorName() const {
    assert(isUnaryOp() || isBinaryOp());
    return Name[Name.size() - 1];
  }
  unsigned getBinaryPrecedence() const { return Precedence; }
  int getLine() const { return Line; }
  std::unique_ptr<PrototypeAST> clone(){
    return std::make_unique<PrototypeAST>((Source_location){Line, 0}, Name, Args, cpoint_type, IsOperator, Precedence, is_variable_number_args, has_template, template_name);
  }
};

class FunctionAST {
public:
  std::unique_ptr<PrototypeAST> Proto;
  std::vector<std::unique_ptr<ExprAST>> Body;
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::vector<std::unique_ptr<ExprAST>> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
  Function *codegen();
  std::unique_ptr<FunctionAST> clone(){
    std::vector<std::unique_ptr<ExprAST>> BodyClone;
    for (int i = 0; i < Body.size(); i++){
      BodyClone.push_back(Body.at(i)->clone());
    }
    return std::make_unique<FunctionAST>(Proto->clone(), std::move(BodyClone));
  }
};

class StructDeclarAST {
public:
  std::string Name;
  std::vector<std::unique_ptr<VarExprAST>> Vars;
  std::vector<std::unique_ptr<FunctionAST>> Functions;
  std::vector<std::unique_ptr<PrototypeAST>> ExternFunctions;
  bool has_template;
  std::string template_name;
  StructDeclarAST(const std::string &name, std::vector<std::unique_ptr<VarExprAST>> Vars, std::vector<std::unique_ptr<FunctionAST>> Functions, std::vector<std::unique_ptr<PrototypeAST>> ExternFunctions, bool has_template, std::string template_name) 
    : Name(name), Vars(std::move(Vars)), Functions(std::move(Functions)), ExternFunctions(std::move(ExternFunctions)), has_template(has_template), template_name(template_name) {}
  Type *codegen();
  std::unique_ptr<StructDeclarAST> clone(){
    std::vector<std::unique_ptr<FunctionAST>> FunctionsCloned;
    if (!Functions.empty()){
      for (int i = 0; i < Functions.size(); i++){
        FunctionsCloned.push_back(Functions.at(i)->clone());
      }
    }
    std::vector<std::unique_ptr<PrototypeAST>> ExternFunctionsCloned;
    if (!ExternFunctions.empty()){
        for (int i = 0; i < ExternFunctions.size(); i++){
            ExternFunctionsCloned.push_back(ExternFunctions.at(i)->clone());
        }
    }
    std::vector<std::unique_ptr<VarExprAST>> VarsCloned;
    Log::Info() << "is Vars empty" << Vars.empty() << "\n"; 
    if (!Vars.empty()){
      for (int i = 0; i < Vars.size(); i++){
        std::unique_ptr<ExprAST> VarCloned = Vars.at(i)->clone();
        std::unique_ptr<VarExprAST> VarTemp;
        VarExprAST* VarTempPtr = dynamic_cast<VarExprAST*>(VarCloned.get());
        if (!VarTempPtr){
            return LogErrorS("Vars in struct is not a var and is an other type exprAST");
        }
        VarCloned.release();
        VarTemp.reset(VarTempPtr);
        VarsCloned.push_back(std::move(VarTemp));
      }
    }
    return std::make_unique<StructDeclarAST>(Name, std::move(VarsCloned), std::move(FunctionsCloned), std::move(ExternFunctionsCloned), has_template, template_name);
  }
};

class UnionDeclarAST {
public:
  std::string Name;
  std::vector<std::unique_ptr<VarExprAST>> Vars;
  UnionDeclarAST(const std::string& Name, std::vector<std::unique_ptr<VarExprAST>> Vars) : Name(Name), Vars(std::move(Vars)) {}
  Type* codegen();
  std::unique_ptr<UnionDeclarAST> clone(){
    std::vector<std::unique_ptr<VarExprAST>> VarsCloned;
    for (int i = 0; i < Vars.size(); i++){
      std::unique_ptr<ExprAST> VarCloned = Vars.at(i)->clone();
      std::unique_ptr<VarExprAST> VarTemp;
      VarExprAST* VarTempPtr = static_cast<VarExprAST*>(VarCloned.get());
      VarCloned.release();
      VarTemp.reset(VarTempPtr);
      VarsCloned.push_back(std::move(VarTemp));
    }
    return std::make_unique<UnionDeclarAST>(Name, std::move(VarsCloned));
  }
};

class EnumMember {
public:
    std::string Name;
    bool contains_value;
    std::unique_ptr<Cpoint_Type> Type;
    EnumMember(const std::string& Name, bool contains_value = false, std::unique_ptr<Cpoint_Type> Type = nullptr) : Name(Name), contains_value(contains_value), Type(std::move(Type)) {}
    std::unique_ptr<EnumMember> clone(){
        std::unique_ptr<Cpoint_Type> TypeCloned = nullptr;
        if (Type){
            TypeCloned = std::make_unique<Cpoint_Type>(*Type);
        }
        return std::make_unique<EnumMember>(Name, contains_value, std::move(TypeCloned));
    }
};

class EnumDeclarAST {
public:
    std::string Name;
    bool enum_member_contain_type;
    std::vector<std::unique_ptr<EnumMember>> EnumMembers;
    EnumDeclarAST(const std::string& Name, bool enum_member_contain_type, std::vector<std::unique_ptr<EnumMember>> EnumMembers) : Name(Name), enum_member_contain_type(enum_member_contain_type), EnumMembers(std::move(EnumMembers)) {}
    Type* codegen();
    std::unique_ptr<EnumDeclarAST> clone(){
        std::vector<std::unique_ptr<EnumMember>> EnumMembersCloned;
        if (!EnumMembers.empty()){
            for (int i = 0; i < EnumMembers.size(); i++){
                EnumMembersCloned.push_back(EnumMembers.at(i)->clone());
            }
        }
        return std::make_unique<EnumDeclarAST>(Name, enum_member_contain_type, std::move(EnumMembersCloned));
    }
};

class TestAST {
public:
  std::string description;
  std::vector<std::unique_ptr<ExprAST>> Body;
  TestAST(const std::string& description, std::vector<std::unique_ptr<ExprAST>> Body) : description(description), Body(std::move(Body)) {}
  void codegen();
  std::unique_ptr<TestAST> clone(){
    std::vector<std::unique_ptr<ExprAST>> BodyCloned;
    if (!Body.empty()){
      for (int i = 0; i < Body.size(); i++){
        BodyCloned.push_back(Body.at(i)->clone());
      }
    }
    return std::make_unique<TestAST>(description, std::move(BodyCloned));
  }
};

class GlobalVariableAST {
public:
  std::string varName;
  bool is_const;
  bool is_extern;
  Cpoint_Type cpoint_type;
  std::unique_ptr<ExprAST> Init;
  GlobalVariableAST(const std::string& varName, bool is_const, bool is_extern, Cpoint_Type cpoint_type, std::unique_ptr<ExprAST> Init) 
    : varName(varName), is_const(is_const), is_extern(is_extern), cpoint_type(cpoint_type), Init(std::move(Init)) {} 
  GlobalVariable* codegen();
};

class TypeDefAST {
public:
  std::string new_type;
  Cpoint_Type value_type;
  TypeDefAST(const std::string& new_type, Cpoint_Type value_type) :  new_type(new_type), value_type(value_type) {}
  void codegen();
};

class CommentExprAST : public ExprAST {
public:
    CommentExprAST() {}
    Value* codegen();
    std::unique_ptr<ExprAST> clone() override {
        return std::make_unique<CommentExprAST>();
    }
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
  IfExprAST(Source_location Loc, std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Then,
            std::vector<std::unique_ptr<ExprAST>> Else)
    : ExprAST(Loc), Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    std::vector<std::unique_ptr<ExprAST>> ThenCloned;
    if (!Then.empty()){
      for (int i = 0; i < Then.size(); i++){
        ThenCloned.push_back(Then.at(i)->clone());
      }
    }
    std::vector<std::unique_ptr<ExprAST>> ElseCloned;
    if (!Else.empty()){
      for (int i = 0; i < Else.size(); i++){
        ElseCloned.push_back(Else.at(i)->clone());
      }
    }
    return std::make_unique<IfExprAST>(ExprAST::loc, Cond->clone(), std::move(ThenCloned), std::move(ElseCloned));
  }
  
};

class ReturnAST : public ExprAST {
  std::unique_ptr<ExprAST> returned_expr;
  //double Val;
public:
  ReturnAST(std::unique_ptr<ExprAST> returned_expr)
  : returned_expr(std::move(returned_expr)) {}
  //ReturnAST(double val) : Val(val) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone(){
    return std::make_unique<ReturnAST>(returned_expr->clone());
  }
};

class BreakExprAST : public ExprAST {
public:
  BreakExprAST() {}
  Value* codegen() override;
  std::unique_ptr<ExprAST> clone(){
    return std::make_unique<BreakExprAST>();
  }
};

class GotoExprAST : public ExprAST {
  std::string label_name;
public:
  GotoExprAST(const std::string& label_name) : label_name(label_name) {}
  Value* codegen() override;
  std::unique_ptr<ExprAST> clone(){
    return std::make_unique<GotoExprAST>(label_name);
  }
};

class LabelExprAST : public ExprAST {
  std::string label_name;
public:
  LabelExprAST(const std::string& label_name) : label_name(label_name) {}
  Value* codegen() override;
  std::unique_ptr<ExprAST> clone(){
    return std::make_unique<LabelExprAST>(label_name);
  }
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
  std::unique_ptr<ExprAST> clone(){
    std::vector<std::unique_ptr<ExprAST>> BodyCloned;
    if (!Body.empty()){
      for (int i = 0; i < Body.size(); i++){
        BodyCloned.push_back(Body.at(i)->clone());
      }
    }
    return std::make_unique<ForExprAST>(VarName, Start->clone(), End->clone(), Step->clone(), std::move(BodyCloned));
  }
};

class WhileExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond;
  std::vector<std::unique_ptr<ExprAST>> Body;
public:
  WhileExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Body) : Cond(std::move(Cond)), Body(std::move(Body)) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone(){
    std::vector<std::unique_ptr<ExprAST>> BodyCloned;
    if (!Body.empty()){
      for (int i = 0; i < Body.size(); i++){
        BodyCloned.push_back(Body.at(i)->clone());
      }
    }
    return std::make_unique<WhileExprAST>(Cond->clone(), std::move(BodyCloned));
  }
};

class LoopExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Array;
  std::vector<std::unique_ptr<ExprAST>> Body;
  bool is_infinite_loop;
public:
  LoopExprAST(std::string VarName, std::unique_ptr<ExprAST> Array, std::vector<std::unique_ptr<ExprAST>> Body, bool is_infinite_loop = false) : VarName(VarName), Array(std::move(Array)), Body(std::move(Body)), is_infinite_loop(is_infinite_loop) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone(){
    std::vector<std::unique_ptr<ExprAST>> BodyCloned;
    if (!Body.empty()){
      for (int i = 0; i < Body.size(); i++){
        BodyCloned.push_back(Body.at(i)->clone());
      }
    }
    return std::make_unique<LoopExprAST>(VarName, Array->clone(), std::move(BodyCloned), is_infinite_loop);
  }
};

std::unique_ptr<ExprAST> ParseExpression();
std::unique_ptr<ExprAST> ParsePrimary();
std::unique_ptr<FunctionAST> ParseDefinition();
std::unique_ptr<PrototypeAST> ParseExtern();
std::unique_ptr<ExprAST> ParseFunctionArgs(std::vector<std::unique_ptr<ExprAST>>& Args);
std::unique_ptr<ExprAST> ParseBodyExpressions(std::vector<std::unique_ptr<ExprAST>>& Body, bool is_func_body = false);
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
std::unique_ptr<ExprAST> ParseTypeidExpr();
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
std::unique_ptr<ExprAST> ParseNullExpr();
std::unique_ptr<ExprAST> ParseCastExpr();
std::unique_ptr<ModAST> ParseMod();
std::unique_ptr<TestAST> ParseTest();
std::unique_ptr<UnionDeclarAST> ParseUnion();
std::unique_ptr<EnumDeclarAST> ParseEnum();

std::unique_ptr<ExprAST> vLogError(const char* Str, va_list args, Source_location astLoc);
std::unique_ptr<ExprAST> LogError(const char *Str, ...);

#endif
