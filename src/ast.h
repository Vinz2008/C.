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
#include <cmath>

using namespace llvm;

#ifndef _AST_HEADER_
#define _AST_HEADER_

#include "types.h"
#include "errors.h"
#include "log.h"
#include "c_translator.h"
#include "config.h"

extern std::unique_ptr<Compiler_context> Comp_context; 

class StructDeclarAST;

template <class T>
std::unique_ptr<T> get_Expr_from_ExprAST(std::unique_ptr<ExprAST> E);

//std::unique_ptr<StructDeclarAST> LogErrorS(const char *Str, ...);

class ExprAST {
public:
  ExprAST(Source_location loc = Comp_context->lexloc) : loc(loc) {}
  Source_location loc;
  virtual ~ExprAST() = default;
  virtual Value *codegen() = 0;
  virtual std::unique_ptr<ExprAST> clone() = 0;
  virtual std::string to_string() = 0;
  virtual std::string generate_c() = 0;
  int getLine() const { return loc.line_nb; }
  int getCol() const { return loc.col_nb; }
};

class EmptyExprAST : public ExprAST {
public:
    EmptyExprAST(){};
    Value *codegen() override { return nullptr; }
    std::unique_ptr<ExprAST> clone() override { return std::make_unique<EmptyExprAST>(); }
    std::string to_string() override {
        return "";
    }
    std::string generate_c() override { return ""; }
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
public:
  double Val;
  NumberExprAST(double Val) : Val(Val) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<NumberExprAST>(Val);
  }
  std::string to_string() override {
    if (std::trunc(Val) == Val){
        return std::to_string((int)Val);
    }
    return std::to_string(Val);
  }
  std::string generate_c() override;
};

class StringExprAST : public ExprAST {
public:
  std::string str;
  StringExprAST(const std::string &str) : str(str) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<StringExprAST>(str);
  }
  std::string to_string() override {
    return "\"" + str + "\"";
  }
  std::string generate_c() override;
};

class CharExprAST : public ExprAST {
  int c;
public:
  CharExprAST(int c) : c(c) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<CharExprAST>(c);
  }
  std::string to_string() override {
    return "\'" + std::string(1, (char)c) + "\'";
  }
  std::string generate_c() override;
};

class BoolExprAST : public ExprAST {
  bool val;
public:
  BoolExprAST(bool val) : val(val) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<BoolExprAST>(val);
  }
  std::string to_string() override {
    return val ? "true" : "false";
  }
  std::string generate_c() override;
};

class AddrExprAST : public ExprAST {
  //std::string Ident;
  std::unique_ptr<ExprAST> Expr;
public:
  //AddrExprAST(const std::string &Name) : Name(Name) {}
  AddrExprAST(/*const std::string &Ident,*/ std::unique_ptr<ExprAST> Expr) : /*Ident(Ident),*/ Expr(std::move(Expr)) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<AddrExprAST>(/*Ident,*/ Expr->clone());
  }
  std::string to_string() override {
    return "addr " + /*((Expr) ?*/ Expr->to_string() /*: Ident)*/;
  }
  std::string generate_c() override;
};

class DerefExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Expr;
public:
    DerefExprAST(std::unique_ptr<ExprAST> Expr) : Expr(std::move(Expr)) {}
    Value *codegen() override;
    std::unique_ptr<ExprAST> clone() override {
        return std::make_unique<DerefExprAST>(Expr->clone());
    }
    std::string to_string() override {
        return "deref " + Expr->to_string();
    }
    std::string generate_c() override;
};

// TODO : fix sizeof not being printed in assertions with expect (to_string() return "" ?)
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
  std::string to_string() override {
    return "sizeof " + Name != "" ? Name : create_pretty_name_for_type(type);
  }
  std::string generate_c() override;
};

class TypeidExprAST : public ExprAST {
    std::unique_ptr<ExprAST> val;
public:
    TypeidExprAST(std::unique_ptr<ExprAST> val) : val(std::move(val)) {}
    Value* codegen() override;
    std::unique_ptr<ExprAST> clone() override {
        return std::make_unique<TypeidExprAST>(val->clone());
    }

    std::string to_string() override {
        return "typeId " + val->to_string();
    }
    std::string generate_c() override { return ""; }
};

class ArgInlineAsm {
public:
    std::unique_ptr<ExprAST> ArgExpr;
    enum ArgType {
        output,
        input
    } argType;
    ArgInlineAsm(std::unique_ptr<ExprAST> ArgExpr, enum ArgType argType) : ArgExpr(std::move(ArgExpr)), argType(argType) {}
    std::unique_ptr<ArgInlineAsm> clone(){
        return std::make_unique<ArgInlineAsm>(ArgExpr->clone(), argType);
    }
};

class ArgsInlineAsm {
public:
    std::unique_ptr<StringExprAST> assembly_code;
    std::vector<std::unique_ptr<ArgInlineAsm>> InputOutputArgs;
    ArgsInlineAsm(std::unique_ptr<StringExprAST> assembly_code, std::vector<std::unique_ptr<ArgInlineAsm>> InputOutputArgs) : assembly_code(std::move(assembly_code)), InputOutputArgs(std::move(InputOutputArgs)) {}
    std::unique_ptr<ArgsInlineAsm> clone();
};

class AsmExprAST : public ExprAST {
    //std::string assembly_code;
    std::unique_ptr<ArgsInlineAsm> Args;
public:
    AsmExprAST(std::unique_ptr<ArgsInlineAsm> Args) : Args(std::move(Args)) {}
    Value* codegen() override;
    std::unique_ptr<ExprAST> clone() override {
        return std::make_unique<AsmExprAST>(Args->clone());
    }
    std::string to_string() override {
        return "#asm(\"" + Args->assembly_code->str + "\")";
    }
    std::string generate_c() override;
};
class VariableExprAST : public ExprAST {
public:
  std::string Name;
  Cpoint_Type type;
  VariableExprAST(Source_location Loc, const std::string &Name, Cpoint_Type type) : ExprAST(Loc), Name(Name), type(type) {}
  Value *codegen() override;
  const std::string &getName() const { return Name; }
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<VariableExprAST>(ExprAST::loc, Name, type);
  }
  std::string to_string() override {
    return Name;
  }
  std::string generate_c() override;
};

class UnionMemberExprAST : public ExprAST {
public:
  std::string UnionName;
  std::string MemberName;
  UnionMemberExprAST(const std::string &UnionName, const std::string &MemberName) : UnionName(UnionName), MemberName(MemberName) {}
  Value* codegen();
  std::unique_ptr<ExprAST> clone(){
    return std::make_unique<UnionMemberExprAST>(UnionName, MemberName);
  }
  std::string to_string() override {
    return UnionName + "." + MemberName;
  }
  std::string generate_c() override { return ""; }
};

class ConstantArrayExprAST : public ExprAST {
public:
  std::vector<std::unique_ptr<ExprAST>> ArrayMembers;
  ConstantArrayExprAST(std::vector<std::unique_ptr<ExprAST>> ArrayMembers) : ArrayMembers(std::move(ArrayMembers)) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override;
  std::string to_string() override {
    std::string array = "";
    for (int i = 0; i < ArrayMembers.size(); i++){
        array += ArrayMembers.at(i)->to_string() + ",";
    }
    return "[" + array + "]";
  }
  std::string generate_c() override { return ""; }
};

class ConstantStructExprAST : public ExprAST {
public:
    std::string struct_name;
    std::vector<std::unique_ptr<ExprAST>> StructMembers;
    ConstantStructExprAST(const std::string& struct_name, std::vector<std::unique_ptr<ExprAST>> StructMembers) : struct_name(struct_name), StructMembers(std::move(StructMembers)) {}
    Value* codegen() override;
    std::unique_ptr<ExprAST> clone() override;
    std::string to_string() override {
        std::string struct_members_str = "";
        for (int i = 0; i < StructMembers.size(); i++){
            struct_members_str += StructMembers.at(i)->to_string();
        }
        return struct_name + " { " + struct_members_str + " }";
    }
    std::string generate_c() override { return ""; }
};

class EnumCreation : public ExprAST {
public:
    std::string EnumVarName;
    std::string EnumMemberName;
    std::unique_ptr<ExprAST> value;
    EnumCreation(const std::string& EnumVarName, const std::string& EnumMemberName, std::unique_ptr<ExprAST> value) : EnumVarName(EnumVarName), EnumMemberName(EnumMemberName), value(std::move(value)) {}
    llvm::Value* codegen() override;
    //Value *codegen() override;
    std::unique_ptr<ExprAST> clone() override;
    std::string to_string() override {
        return EnumVarName + "::" + EnumMemberName + "(" + ((value) ? value->to_string() : (std::string)"") + ")";
    }
    std::string generate_c() override { return ""; }
};

class BinaryExprAST : public ExprAST {
public:
  std::string Op;
  std::unique_ptr<ExprAST> LHS, RHS;
  BinaryExprAST(Source_location Loc, const std::string& op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
    : ExprAST(Loc), Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<BinaryExprAST>(ExprAST::loc, Op, LHS->clone(), RHS->clone());
  }
  std::string to_string() override {
    return LHS->to_string() + " " + Op + " " + RHS->to_string();  
  }
  std::string generate_c() override;
};

class UnaryExprAST : public ExprAST {
public:
  char Opcode;
  std::unique_ptr<ExprAST> Operand;
  UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
    : Opcode(Opcode), Operand(std::move(Operand)) {}

  Value *codegen() override;
  std::unique_ptr<ExprAST> clone() override {
    return std::make_unique<UnaryExprAST>(Opcode, Operand->clone());
  }
  std::string to_string() override {
    return Opcode + Operand->to_string();
  }
  std::string generate_c() override;
};

struct StructMemberCallExprAST : public ExprAST {
    std::unique_ptr<BinaryExprAST> StructMember;
    std::vector<std::unique_ptr<ExprAST>> Args;
    StructMemberCallExprAST(std::unique_ptr<BinaryExprAST> StructMember, std::vector<std::unique_ptr<ExprAST>> Args) : StructMember(std::move(StructMember)), Args(std::move(Args)) {}
    std::string to_string() override {
        return StructMember->LHS->to_string() + "." + StructMember->RHS->to_string();
    }
    std::string generate_c() override { return ""; }
    std::unique_ptr<ExprAST> clone() override;
    Value *codegen() override;
};

struct NEWCallExprAST : public ExprAST {
    std::unique_ptr<ExprAST> function_expr;
    std::vector<std::unique_ptr<ExprAST>> Args;
    Cpoint_Type template_passed_type;
    NEWCallExprAST(std::unique_ptr<ExprAST> function_expr, std::vector<std::unique_ptr<ExprAST>> Args, Cpoint_Type template_passed_type) : function_expr(std::move(function_expr)), Args(std::move(Args)), template_passed_type(template_passed_type) {}
    std::string generate_c() override { return ""; }
    std::string to_string() override {
        return "";
    }
    std::unique_ptr<ExprAST> clone() override;
    Value *codegen() override;
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
  std::unique_ptr<ExprAST> clone() override;
  std::string to_string() override {
    std::string function_name = Callee;
    if (function_name.find("___") != std::string::npos){
        std::string mangled_function_name = "";
        int underscore_number = 0;
        for (int i = 0; i < function_name.size(); i++){
            if (function_name.at(i) == '_'){
                underscore_number++;
            } else {
                if (underscore_number == 1){
                    mangled_function_name += '_';
                }
                if (underscore_number == 2){
                // TODO : how to find struct name on which the function is used
                mangled_function_name += "__";
                underscore_number = 0;
                }
                mangled_function_name += function_name.at(i);
                underscore_number = 0;
            }
            if (underscore_number == 3){
                mangled_function_name += "::";
                underscore_number = 0;
            }
        }
        function_name = mangled_function_name;
    }
    std::string args = "";
    args += "(";
    for (int i = 0; i < Args.size(); i++){
        args += Args.at(i)->to_string() /*+ ","*/;
        if (i != Args.size()-1){
            args += ",";
        }
    }
    args += ")";
    std::string template_type = "";
    // TODO maybe add the template type to the string expression (add a bool in this class to identify if a type is passed in the template)
    return function_name + template_type + args;
  }
  std::string generate_c() override;
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
  std::unique_ptr<ExprAST> clone() override;
  std::string to_string() override {
    std::string var_expr =  "var " + VarNames.at(0).first + ": " + create_pretty_name_for_type(cpoint_type); 
    if (VarNames.at(0).second){
        var_expr += " = " + VarNames.at(0).second->to_string();
    }
    return var_expr;
  }
  std::string generate_c() override;
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
  std::unique_ptr<ExprAST> clone() override;
  std::string to_string() override {
    std::string member_expr  = member != "" ? "." + member : "";
    return VariableName + member_expr + " = " + Val->to_string();
  }
  std::string generate_c() override;
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
    std::string to_string() override {
      return "cast " + create_pretty_name_for_type(type) + " " + ValToCast->to_string();
    }
    std::string generate_c() override;
};

class NullExprAST : public ExprAST {
public:
    NullExprAST() {}
    Value *codegen() override;
    std::unique_ptr<ExprAST> clone(){
        return std::make_unique<NullExprAST>();
    }
    std::string to_string() override {
        return "null";
    }
    std::string generate_c() override;
};

// JUNKY HACK TIME !!
// will just return a Value* when being codegened
// is used if a value* is needed to be passed as an AST node
class LLVMASTValueWrapper : public ExprAST {
public:
    Value* val;
    LLVMASTValueWrapper(Value* val) : val(val){}
    Value* codegen() {
        return val;
    }
    std::unique_ptr<ExprAST> clone(){
        return std::make_unique<LLVMASTValueWrapper>(val);
    }
    std::string to_string() {
        return "";
    }
    std::string generate_c(){
        return ""; // How will we fucking use this with the c backend ?? No idea, will figure this out
    }
};

class matchCase {
public:
    std::unique_ptr<ExprAST> expr;
    std::string enum_name;
    std::string enum_member;
    std::string var_name;
    bool is_underscore;
    std::vector<std::unique_ptr<ExprAST>> Body; 
    matchCase(std::unique_ptr<ExprAST> expr, const std::string& enum_name, const std::string& enum_member, const std::string& var_name, bool is_underscore, std::vector<std::unique_ptr<ExprAST>> Body) : expr(std::move(expr)), enum_name(enum_name), enum_member(enum_member), var_name(var_name), is_underscore(is_underscore), Body(std::move(Body)) {}
    std::unique_ptr<matchCase> clone();
};

class MatchExprAST : public ExprAST {
public:
    std::string matchVar;
    std::vector<std::unique_ptr<matchCase>> matchCases;
    MatchExprAST(const std::string& matchVar, std::vector<std::unique_ptr<matchCase>> matchCases) : matchVar(matchVar), matchCases(std::move(matchCases)) {}
    Value *codegen() override;
    std::unique_ptr<ExprAST> clone();
    std::string to_string() override {
        // TODO
        return "";
    }
    std::string generate_c() override { return ""; }
};

class ClosureAST : public ExprAST {
    std::vector<std::unique_ptr<ExprAST>> Body;
    std::vector<std::pair<std::string, Cpoint_Type>> ArgNames;
    Cpoint_Type return_type;
    std::vector<std::string> captured_vars;
public:
    ClosureAST(std::vector<std::unique_ptr<ExprAST>> Body, std::vector<std::pair<std::string, Cpoint_Type>> ArgNames, Cpoint_Type return_type, std::vector<std::string> captured_vars) : Body(std::move(Body)), ArgNames(ArgNames), return_type(return_type), captured_vars(captured_vars) {}
    Value *codegen() override;
    std::unique_ptr<ExprAST> clone();
    std::string to_string() override {
        // TODO
        return "";
    }
    std::string generate_c() override { return ""; }
};

// TODO : fix the reorder warning
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
  c_translator::Function* c_codegen();
};

class FunctionAST {
public:
  std::unique_ptr<PrototypeAST> Proto;
  std::vector<std::unique_ptr<ExprAST>> Body;
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::vector<std::unique_ptr<ExprAST>> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
  Function *codegen();
  c_translator::Function* c_codegen();
  std::unique_ptr<FunctionAST> clone();
  // for debug
  // is a way to see the ast
  // maybe have a real tree or json printing for ast later
  std::string get_ast_string(){
    std::string ret = "";
    for (int i = 0; i < Body.size(); i++){
        ret += Body.at(i)->to_string() + "\n";
    }
    return ret;
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
  std::unique_ptr<StructDeclarAST> clone();
};

class UnionDeclarAST {
public:
  std::string Name;
  std::vector<std::unique_ptr<VarExprAST>> Vars;
  UnionDeclarAST(const std::string& Name, std::vector<std::unique_ptr<VarExprAST>> Vars) : Name(Name), Vars(std::move(Vars)) {}
  Type* codegen();
  std::unique_ptr<UnionDeclarAST> clone();
};

class MembersDeclarAST {
public:
    std::string members_name; // to add trait system like in rust ? TODO ?
    std::string members_for;
    std::vector<std::unique_ptr<FunctionAST>> Functions;
    std::vector<std::unique_ptr<PrototypeAST>> Externs;
    MembersDeclarAST(const std::string& members_name, const std::string& members_for, std::vector<std::unique_ptr<FunctionAST>> Functions, std::vector<std::unique_ptr<PrototypeAST>> Externs) : members_name(members_name), members_for(members_for), Functions(std::move(Functions)), Externs(std::move(Externs)) {}
    void codegen();
};

class EnumMember {
public:
    std::string Name;
    bool contains_value;
    std::unique_ptr<Cpoint_Type> Type;
    bool contains_custom_index;
    int Index;
    EnumMember(const std::string& Name, bool contains_value = false, std::unique_ptr<Cpoint_Type> Type = nullptr, bool contains_custom_index = false, int Index = -1) : Name(Name), contains_value(contains_value), Type(std::move(Type)), contains_custom_index(contains_custom_index), Index(Index) {}
    std::unique_ptr<EnumMember> clone();
};

class EnumDeclarAST {
public:
    std::string Name;
    bool enum_member_contain_type;
    std::vector<std::unique_ptr<EnumMember>> EnumMembers;
    EnumDeclarAST(const std::string& Name, bool enum_member_contain_type, std::vector<std::unique_ptr<EnumMember>> EnumMembers) : Name(Name), enum_member_contain_type(enum_member_contain_type), EnumMembers(std::move(EnumMembers)) {}
    Type* codegen();
    std::unique_ptr<EnumDeclarAST> clone();
};

class TestAST {
public:
  std::string description;
  std::vector<std::unique_ptr<ExprAST>> Body;
  TestAST(const std::string& description, std::vector<std::unique_ptr<ExprAST>> Body) : description(description), Body(std::move(Body)) {}
  void codegen();
  std::unique_ptr<TestAST> clone();
};

class GlobalVariableAST {
public:
  std::string varName;
  bool is_const;
  bool is_extern;
  Cpoint_Type cpoint_type;
  std::unique_ptr<ExprAST> Init;
  bool is_array;
  std::unique_ptr<ExprAST> index;
  std::string section_name;
  GlobalVariableAST(const std::string& varName, bool is_const, bool is_extern, Cpoint_Type cpoint_type, std::unique_ptr<ExprAST> Init, bool is_array, std::unique_ptr<ExprAST> index, std::string section_name) 
    : varName(varName), is_const(is_const), is_extern(is_extern), cpoint_type(cpoint_type), Init(std::move(Init)), is_array(is_array), index(std::move(index)), section_name(section_name) {} 
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
    std::string to_string() override {
        return "";
    }
    std::string generate_c() override { return ""; }
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
  std::unique_ptr<ExprAST> clone() override;
  std::string to_string() override {
    std::string body_then = "";
    for (int i = 0; i < Then.size(); i++){
        body_then += Then.at(i)->to_string() + "\n";
    }
    std::string body_else = "";
    for (int i = 0; i < Else.size(); i++){
        body_else += Else.at(i)->to_string() + "\n";
    }
    return "if " + Cond->to_string() + "{\n" + body_then + "} else {\n" + body_else + "}";
  }
  std::string generate_c() override;
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
  std::string to_string() override {
    return "return " + returned_expr->to_string();
  }
  std::string generate_c() override;
};

class BreakExprAST : public ExprAST {
public:
  BreakExprAST() {}
  Value* codegen() override;
  std::unique_ptr<ExprAST> clone(){
    return std::make_unique<BreakExprAST>();
  }
  std::string to_string() override {
    return "break";
  }
  std::string generate_c() override;
};

class GotoExprAST : public ExprAST {
  std::string label_name;
public:
  GotoExprAST(const std::string& label_name) : label_name(label_name) {}
  Value* codegen() override;
  std::unique_ptr<ExprAST> clone(){
    return std::make_unique<GotoExprAST>(label_name);
  }
  std::string to_string() override {
    return "goto " + label_name;
  }
  std::string generate_c() override;
};

class LabelExprAST : public ExprAST {
  std::string label_name;
public:
  LabelExprAST(const std::string& label_name) : label_name(label_name) {}
  Value* codegen() override;
  std::unique_ptr<ExprAST> clone(){
    return std::make_unique<LabelExprAST>(label_name);
  }
  std::string to_string() override {
    return label_name + ":";
  }
  std::string generate_c() override;
};

class ForExprAST : public ExprAST {
  std::string VarName;
  Cpoint_Type VarType;
  std::unique_ptr<ExprAST> Start, End, Step;
  std::vector<std::unique_ptr<ExprAST>> Body;

public:
  ForExprAST(const std::string &VarName, Cpoint_Type VarType, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::vector<std::unique_ptr<ExprAST>> Body)
    : VarName(VarName), VarType(VarType), Start(std::move(Start)), End(std::move(End)),
      Step(std::move(Step)), Body(std::move(Body)) {}

  Value *codegen() override;
  std::unique_ptr<ExprAST> clone();
  std::string to_string() override {
    std::string for_body = "";
    for (int i = 0; i < Body.size(); i++){
        for_body += Body.at(i)->to_string() + "\n";
    }
    return "for " + VarName + " = " + Start->to_string() + ", " + End->to_string() + ", " + Step->to_string() + "{\n" + for_body + "}";
  }
	std::string generate_c() override;
};

class WhileExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond;
  std::vector<std::unique_ptr<ExprAST>> Body;
public:
  WhileExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Body) : Cond(std::move(Cond)), Body(std::move(Body)) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone();
  std::string to_string() override {
    std::string while_body = "";
    for (int i = 0; i < Body.size(); i++){
        while_body += Body.at(i)->to_string() + "\n";
    }
    return "while" + Cond->to_string() + " {\n" + while_body + "}";
  }
  std::string generate_c() override;
};

class LoopExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Array;
  std::vector<std::unique_ptr<ExprAST>> Body;
  bool is_infinite_loop;
public:
  LoopExprAST(std::string VarName, std::unique_ptr<ExprAST> Array, std::vector<std::unique_ptr<ExprAST>> Body, bool is_infinite_loop = false) : VarName(VarName), Array(std::move(Array)), Body(std::move(Body)), is_infinite_loop(is_infinite_loop) {}
  Value *codegen() override;
  std::unique_ptr<ExprAST> clone();
  std::string to_string() override {
    std::string loop_expr = "";
    if (!is_infinite_loop){
        loop_expr = VarName + " in " + Array->to_string();
    }
    std::string loop_body = "";
    for (int i = 0; i < Body.size(); i++){
        loop_body += Body.at(i)->to_string() + "\n";
    }
    return "loop " +  loop_expr + "{\n" + loop_body + "}";
  }
  std::string generate_c() override;
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
std::unique_ptr<MembersDeclarAST> ParseMembers();
std::unique_ptr<ExprAST> ParseAddrExpr();
std::unique_ptr<ExprAST> ParseDerefExpr();
std::unique_ptr<ExprAST> ParseSizeofExpr();
std::unique_ptr<ExprAST> ParseTypeidExpr();
std::unique_ptr<ExprAST> ParseArrayMemberExpr();
std::unique_ptr<ExprAST> ParseMatch();
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
std::unique_ptr<ExprAST> ParseMacroCall();
std::unique_ptr<ExprAST> ParseClosure();

std::unique_ptr<ExprAST> vLogError(const char* Str, va_list args, Source_location astLoc);
std::unique_ptr<ExprAST> LogError(const char *Str, ...);

template <class T>
std::vector<std::unique_ptr<T>> clone_vector(std::vector<std::unique_ptr<T>>& v);

void generate_gc_init(std::vector<std::unique_ptr<ExprAST>>& Body);
std::unique_ptr<VarExprAST> get_VarExpr_from_ExprAST(std::unique_ptr<ExprAST> E);

template <class T>
std::unique_ptr<T> get_Expr_from_ExprAST(std::unique_ptr<ExprAST> E){
    std::unique_ptr<T> SubClass;
    T* SubClassPtr = dynamic_cast<T*>(E.get());
    if (!SubClassPtr){
        return nullptr;
    }
    E.release();
    SubClass.reset(SubClassPtr);
    return SubClass;
}

#endif
