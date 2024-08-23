#include <unordered_map>
#include <iostream>
#include <utility>
#include <cstdarg>
#include <cstdlib>
#include <ctime>
#include <stack>
#include "ast.h"
#include "lexer.h"
#include "types.h"
#include "errors.h"
#include "log.h"
#include "codegen.h"
#include "config.h"
#include "preprocessor.h"
#include "macros.h"
#include "abi.h"
#include "debuginfo.h"

extern double NumVal;
extern int CurTok;
extern std::string strStatic;
extern int charStatic;
extern std::string IdentifierStr;
extern int return_status;
extern std::string strPosArray;
extern std::string OpStringMultiChar;
extern int posArrayNb;
extern std::unique_ptr<Compiler_context> Comp_context;
extern std::unordered_map<std::string, std::unique_ptr<NamedValue>> NamedValues;
extern std::unordered_map<std::string, std::unique_ptr<GlobalVariableValue>> GlobalVariables;
extern std::unordered_map<std::string, std::unique_ptr<EnumDeclaration>> EnumDeclarations;
extern std::unique_ptr<Module> TheModule;
extern std::vector<std::string> types_list;
extern std::vector<Cpoint_Type> typeDefTable;

extern std::unordered_map<std::string, std::unique_ptr<TemplateProto>> TemplateProtos;
extern std::unordered_map<std::string, std::unique_ptr<StructDeclar>> TemplateStructDeclars;
extern std::unordered_map<std::string, std::unique_ptr<EnumDeclar>> TemplateEnumDeclars;
//extern std::vector<std::string> modulesNamesContext;
extern std::pair<std::string, Cpoint_Type> TypeTemplateCallCodegen;
extern std::vector<std::unique_ptr<TemplateStructCreation>> StructTemplatesToGenerate;
extern std::vector<std::unique_ptr<TemplateEnumCreation>> EnumTemplatesToGenerate;
extern std::string TypeTemplateCallAst;
extern std::unordered_map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;

extern std::unique_ptr<Preprocessor::Context> context;

Cpoint_Type ParseTypeDeclaration(bool eat_token = true, bool is_return = false, bool no_default_type = false);

bool is_comment = false;

extern void HandleComment();
extern std::string filename;

bool is_template_parsing_definition = false;
bool is_template_parsing_struct = false;
bool is_template_parsing_enum = false;

Source_location emptyLoc = {0, 0, true, ""};

template <class T>
std::vector<std::unique_ptr<T>> clone_vector(std::vector<std::unique_ptr<T>>& v){
    std::vector<std::unique_ptr<T>> v_cloned;
    if (!v.empty()){
        for (int i = 0; i < v.size(); i++){
            v_cloned.push_back(v.at(i)->clone());
        }
    }
    return v_cloned;
}

template<typename T, typename ... Ptrs>
std::vector<std::unique_ptr<T>> make_unique_ptr_static_vector(Ptrs&& ... ptrs){
    std::vector<std::unique_ptr<T>> vec;
    (vec.emplace_back( std::forward<Ptrs>(ptrs) ), ...);
    return vec;
}

std::unique_ptr<ExprAST> StructMemberCallExprAST::clone(){
    return std::make_unique<StructMemberCallExprAST>(get_Expr_from_ExprAST<BinaryExprAST>(StructMember->clone()), clone_vector<ExprAST>(Args));
}

std::unique_ptr<ExprAST> ConstantArrayExprAST::clone(){
    return std::make_unique<ConstantArrayExprAST>(clone_vector<ExprAST>(ArrayMembers));
}

std::unique_ptr<ExprAST> ConstantStructExprAST::clone(){
    return std::make_unique<ConstantStructExprAST>(struct_name, clone_vector<ExprAST>(StructMembers));
}

std::unique_ptr<ExprAST> ConstantVectorExprAST::clone(){
    return std::make_unique<ConstantVectorExprAST>(vector_element_type, vector_size, clone_vector<ExprAST>(VectorMembers));
}

std::unique_ptr<ExprAST> EnumCreation::clone(){
    std::unique_ptr<ExprAST> valueCloned = nullptr;
    if (value){
        valueCloned = value->clone();
    }
    return std::make_unique<EnumCreation>(EnumVarName, EnumMemberName, std::move(valueCloned));
}

std::unique_ptr<ExprAST> CallExprAST::clone(){
    return std::make_unique<CallExprAST>(ExprAST::loc, Callee, clone_vector<ExprAST>(Args), template_passed_type);
}

std::unique_ptr<ExprAST> VarExprAST::clone(){
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNamesCloned;
    if (!VarNames.empty()){
      Log::Info() << "VarNames.size() " << VarNames.size() << "\n";
      for (int i = 0; i < VarNames.size(); i++){
        VarNamesCloned.push_back(std::make_pair(VarNames.at(i).first, VarNames.at(i).second ? VarNames.at(i).second->clone() : nullptr));
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

std::unique_ptr<FunctionAST> FunctionAST::clone(){
    return std::make_unique<FunctionAST>(Proto->clone(), clone_vector<ExprAST>(Body));
}

std::unique_ptr<StructDeclarAST> StructDeclarAST::clone(){
    std::vector<std::unique_ptr<VarExprAST>> VarsCloned;
    Log::Info() << "is Vars empty" << Vars.empty() << "\n"; 
    if (!Vars.empty()){
      for (int i = 0; i < Vars.size(); i++){
        std::unique_ptr<VarExprAST> VarTemp = get_Expr_from_ExprAST<VarExprAST>(Vars.at(i)->clone());
        assert(VarTemp != nullptr);

        VarsCloned.push_back(std::move(VarTemp));
      }
    }
    return std::make_unique<StructDeclarAST>(Name, std::move(VarsCloned), clone_vector<FunctionAST>(Functions), clone_vector<PrototypeAST>(ExternFunctions), has_template, template_name);
}

std::unique_ptr<UnionDeclarAST> UnionDeclarAST::clone(){
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

std::unique_ptr<EnumMember> EnumMember::clone(){
    std::unique_ptr<Cpoint_Type> TypeCloned = nullptr;
    if (Type){
        TypeCloned = std::make_unique<Cpoint_Type>(*Type);
    }
    return std::make_unique<EnumMember>(Name, contains_value, std::move(TypeCloned), contains_custom_index, Index);
}

std::unique_ptr<EnumDeclarAST> EnumDeclarAST::clone(){
    return std::make_unique<EnumDeclarAST>(Name, enum_member_contain_type, clone_vector<EnumMember>(EnumMembers), has_template, template_name);
}

std::unique_ptr<TestAST> TestAST::clone(){
    return std::make_unique<TestAST>(description, clone_vector<ExprAST>(Body));
}

std::unique_ptr<ExprAST> IfExprAST::clone(){
    return std::make_unique<IfExprAST>(this->loc, Cond->clone(), Then->clone(), (Else) ? Else->clone() : nullptr);
}

std::unique_ptr<ExprAST> ForExprAST::clone(){
    return std::make_unique<ForExprAST>(VarName, VarType, Start->clone(), End->clone(), Step->clone(), Body->clone());
}

std::unique_ptr<ExprAST> WhileExprAST::clone(){
    return std::make_unique<WhileExprAST>(Cond->clone(), Body->clone());
}

std::unique_ptr<ExprAST> LoopExprAST::clone(){
    return std::make_unique<LoopExprAST>(VarName, Array->clone(), clone_vector<ExprAST>(Body), is_infinite_loop);
}

std::unique_ptr<matchCase> matchCase::clone(){
    std::unique_ptr<ExprAST> exprCloned;
    if (expr){
        exprCloned = expr->clone();
    }
    return std::make_unique<matchCase>(std::move(exprCloned), enum_name, enum_member, var_name, is_underscore, Body->clone());
}

std::unique_ptr<ExprAST> MatchExprAST::clone(){
    return std::make_unique<MatchExprAST>(matchVar, clone_vector<matchCase>(matchCases));
}

std::unique_ptr<ExprAST> ClosureAST::clone(){
    return std::make_unique<ClosureAST>(clone_vector<ExprAST>(Body), ArgNames, return_type,captured_vars);
}

std::unique_ptr<ArgsInlineAsm> ArgsInlineAsm::clone(){
    return std::make_unique<ArgsInlineAsm>(std::unique_ptr<StringExprAST>(dynamic_cast<StringExprAST*>(assembly_code->clone().release())), clone_vector<ArgInlineAsm>(InputOutputArgs));
}

std::unique_ptr<ExprAST> ScopeExprAST::clone(){
    return std::make_unique<ScopeExprAST>(clone_vector<ExprAST>(Body));
}

void generate_gc_init(std::vector<std::unique_ptr<ExprAST>>& Body){
    std::vector<std::unique_ptr<ExprAST>> Args_gc_init;
    auto E_gc_init = std::make_unique<CallExprAST>(emptyLoc, "gc_init", std::move(Args_gc_init), Cpoint_Type());
    Body.push_back(std::move(E_gc_init));
}

// not needed ? (TODO : remove this ?)
bool is_of_expr_type(ExprAST* expr, enum ExprType exprType){
    if (exprType == ExprType::Return){
        return dynamic_cast<ReturnAST*>(expr) != nullptr;
    } else if (exprType == ExprType::Break){
        return dynamic_cast<BreakExprAST*>(expr) != nullptr;
    } else if (exprType == ExprType::Unreachable){
        if (dynamic_cast<CallExprAST*>(expr)){
            CallExprAST* call_expr = dynamic_cast<CallExprAST*>(expr);
            if (call_expr->Callee ==  "cpoint_internal_unreachable"){
                return true;
            }
        }
        return false;
    } else if (exprType == ExprType::NeverFunctionCall){
        if (dynamic_cast<CallExprAST*>(expr)){
            CallExprAST* call_expr = dynamic_cast<CallExprAST*>(expr);
            Log::Info() << "call_expr->Callee : " << call_expr->Callee << "\n";
            return call_expr->get_type().type == never_type;
        }
    }
    return false;
}

static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}

static std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V)
    return nullptr;

  if (CurTok != ')')
    return LogError("expected ')'");
  getNextToken(); // eat ).
  return V;
}

std::unique_ptr<ExprAST> ParseScope(){
    Log::Info() << "PARSING SCOPE" << "\n";
    getNextToken(); // eat '{'
    std::vector<std::unique_ptr<ExprAST>> Body;
    auto ret = ParseBodyExpressions(Body);
    if (!ret){
        return nullptr;
    }
    if (CurTok != '}'){
        return LogError("expected } in scope");
    }
    getNextToken();
    return std::make_unique<ScopeExprAST>(std::move(Body));
}

std::unique_ptr<ExprAST> ParseConstantArray(){
  getNextToken(); // eat '['
  std::vector<std::unique_ptr<ExprAST>> ArrayMembers;
  int member_nb = 0;
  while (true){
    if (CurTok == ']'){
      getNextToken();
      break;
    }
    if (member_nb > 0){
      if (CurTok != ','){
      return LogError("missing \',\' in constant array");
      }
      getNextToken();
    }
    auto E = ParseExpression();
    if (!E){
      return nullptr;
    }
    ArrayMembers.push_back(std::move(E));
    member_nb++;
  }
  return std::make_unique<ConstantArrayExprAST>(std::move(ArrayMembers));
}

int ParseTypeDeclarationVector(bool& is_vector, Cpoint_Type& vector_element_type, int& vector_element_number);

static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;
  struct Source_location IdLoc = Comp_context->curloc;

  Log::Info() << "Parse Identifier Str" << "\n";
  getNextToken();  // eat identifier.
  std::string member = "";
  std::unique_ptr<ExprAST> indexAST = nullptr;
  if (EnumDeclarations[IdName] != nullptr){
    Log::Info() << "enum_name : " << IdName << "\n";
    std::unique_ptr<ExprAST> Value = nullptr;
    if (CurTok != ':'){
        return LogError("expected \"::\" for creation of enum");
    }
    getNextToken();
    if (CurTok != ':'){
        return LogError("expected \"::\" for creation of enum");
    }
    getNextToken();
    std::string memberName = IdentifierStr;
    getNextToken();
    if (CurTok == '('){
        getNextToken();
        Value = ParseExpression();
        if (CurTok != ')'){
            return LogError("expected ')' for the value in the creation of enum");
        }
        getNextToken();
    }
    return std::make_unique<EnumCreation>(IdName, memberName, std::move(Value));
  }

  if (CurTok == ':'){
    getNextToken();
    if (CurTok != ':'){
      return LogError("Invalid single ':'");
    }
    getNextToken();
    std::string module_mangled_function_name = IdName;
    while (CurTok == tok_identifier){
      std::string second_part = IdentifierStr; 
      getNextToken();
      module_mangled_function_name = module_mangling(module_mangled_function_name, second_part);
      if (CurTok != ':'){
      break;
      }
      getNextToken();
      if (CurTok != ':'){
      return LogError("Invalid single ':'");
      }
      getNextToken();
    }
    IdName = module_mangled_function_name;
    Log::Info() << "Mangled call function name : " << IdName << "\n";
  }
  if (StructDeclarations[IdName] != nullptr && CurTok == '{'){
    std::string struct_name = IdName;
    std::vector<std::unique_ptr<ExprAST>> StructMembers;
    getNextToken();
    int member_nb = 0;
    while (true){
        if (CurTok == '}'){
            getNextToken();
            break;
        }
        if (member_nb > 0){
            if (CurTok != ','){
            return LogError("missing \',\' in constant array");
            }
            getNextToken();
        }
        auto E = ParseExpression();
        if (!E){
            return nullptr;
        }
        StructMembers.push_back(std::move(E));
        member_nb++;
    }
    return std::make_unique<ConstantStructExprAST>(struct_name, std::move(StructMembers));
  }

  Log::Info() << "VariableExprAST" << "\n";
  Cpoint_Type* type = get_variable_type(IdName);
  return std::make_unique<VariableExprAST>(IdLoc, IdName, (type) ? *type : Cpoint_Type());
}

std::unique_ptr<ExprAST> getASTNewCallExprAST(std::unique_ptr<ExprAST> function_expr, std::vector<std::unique_ptr<ExprAST>> Args, Cpoint_Type template_passed_type){
    Log::Info() << "NEWCallExprAST codegen" << "\n";
    if (dynamic_cast<VariableExprAST*>(function_expr.get())){
        Log::Info() << "Args size" << Args.size() << "\n";
        std::unique_ptr<VariableExprAST> functionNameExpr = get_Expr_from_ExprAST<VariableExprAST>(std::move(function_expr));
        return std::make_unique<CallExprAST>(emptyLoc, functionNameExpr->Name, std::move(Args), template_passed_type);
    } else if (dynamic_cast<BinaryExprAST*>(function_expr.get())){
        std::unique_ptr<BinaryExprAST> BinExpr = get_Expr_from_ExprAST<BinaryExprAST>(std::move(function_expr));
        Log::Info() << "Doing call on binaryExpr : " << BinExpr->Op << "\n";
        if (BinExpr->Op == "."){
            return std::make_unique<StructMemberCallExprAST>(std::move(BinExpr), std::move(Args));
        } else {
            return LogError("Unknown operator before call () operator");
        }
    }
    return LogError("Trying to call an expression which it is not implemented for");
}

std::unique_ptr<ExprAST> ParseFunctionCallOp(std::unique_ptr<ExprAST> LHS, bool is_template_call){
    Log::Info() << "CallExprAST FOUND" << "\n";
    std::vector<std::unique_ptr<ExprAST>> Args;
    Cpoint_Type template_passed_type;
    if (is_template_call){
        template_passed_type = ParseTypeDeclaration(false);
        if (CurTok != '~'){
            return LogError("expected '~' not found");
        }
        getNextToken(); // passing ~
        if (CurTok != '('){
            return LogError("expected '(' not found");
        }
        getNextToken(); // passing '('
    }
    auto ret = ParseFunctionArgs(Args);
    if (!ret){
        return nullptr;
    }
    Log::Info() << "Args size after parsing : " << Args.size() << "\n";
    if (CurTok != ')'){
        return LogError("Expected ) in call args");
    }
    getNextToken(); // eat ')'
    return getASTNewCallExprAST(std::move(LHS), std::move(Args), template_passed_type);
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  Log::Info() << "PARSING BINOP" << "\n";
  Log::Info() << "ExprPrec : " << ExprPrec << "\n";
  // If this is a binop, find its precedence.
  if (dynamic_cast<SemicolonExprAST*>(LHS.get())){
    return LHS;
  }
  while (true) {
    int TokPrec = 0;
    if (CurTok != tok_op_multi_char){
    TokPrec = GetTokPrecedence();
    Log::Info() << "Tok Precedence not multi char : " << TokPrec << "\n";
    } else {
    TokPrec = getTokPrecedenceMultiChar(OpStringMultiChar);
    Log::Info() << "Tok Precedence multi char " << OpStringMultiChar << " : " << TokPrec << "\n";
    }

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec){
      return LHS;
    }

    // Okay, we know this is a binop.
    std::string BinOp = "";
    Source_location BinLoc = Comp_context->curloc;
    if (CurTok != tok_op_multi_char){
    BinOp += (char)CurTok;
    } else {
      BinOp += OpStringMultiChar;
      getNextToken();
    }
    getNextToken(); // eat binop
    Log::Info() << "BinOP : " << BinOp << "\n";
    std::unique_ptr<ExprAST> RHS;
    std::unique_ptr<ExprAST> FunctionCallExpr;
    bool is_function_call = false;
    if ((BinOp == "(" || BinOp == "~")){
        bool is_template_call = false;
        if (BinOp == "~"){
            is_template_call = true;
        }
        FunctionCallExpr = ParseFunctionCallOp(std::move(LHS), is_template_call);
        Log::Info() << "token after FunctionCall : " << CurTok << "\n";
        is_function_call = true;
    } else {
    // Parse the primary expression after the binary operator.
    if (BinOp == "["){
        RHS = ParseExpression(); // need to be able to use expressions between []
    } else {
        RHS = ParseUnary();
    }
    if (!RHS)
      return nullptr;
    }
    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = -1;
    if (CurTok != tok_op_multi_char){
    if (BinOp == "[" && CurTok == ']'){
        //Log::Info() << "CurTok ARRAY_MEMBER_OPERATOR_IMPL : " << CurTok << "\n";
        //Log::Info() << LHS->to_string() << "[" << RHS->to_string() << "]" << "\n";
        // TODO add better verification for end ']'
        getNextToken(); // pass ']'
    }
    if ((BinOp == "." && CurTok == '=') || (BinOp == "[" && CurTok == '=')){
        NextPrec = 100;
    } else {
        NextPrec = GetTokPrecedence();
    }
    } else {
      NextPrec = getTokPrecedenceMultiChar(OpStringMultiChar);
    }
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }
    // Merge LHS/RHS.
    if (!is_function_call){
        LHS = std::make_unique<BinaryExprAST>(BinLoc, BinOp, std::move(LHS), std::move(RHS));
    } else {
        LHS = std::move(FunctionCallExpr);
    }
  }
}

std::unique_ptr<ExprAST> ParseExpression() {
  Log::Info() << "Parsing Expression" << "\n";
  auto LHS = ParseUnary();
  if (!LHS)
    return nullptr;
  return ParseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<ExprAST> ParsePrimary() {
  Log::Info() << "not operator " << CurTok << " " << IdentifierStr << "\n";
  switch (CurTok) {
  default:
    return LogError("Unknown token %d when expecting an expression", CurTok);
  case '#':
    return ParseMacroCall();
  case ';':
    return ParseSemiColon();
  case tok_single_line_comment:
    HandleComment();
    return std::make_unique<CommentExprAST>();
  case tok_identifier:
    return ParseIdentifierExpr();
  case tok_number:
    return ParseNumberExpr();
  case tok_true:
    return ParseBool(true);
  case tok_false:
    return ParseBool(false);
  case '(':
    return ParseParenExpr();
  case '[':
    return ParseConstantArray();
  case '{':
    return ParseScope();
  case tok_if:
    return ParseIfExpr();
  case tok_return:
    return ParseReturn();
  case tok_for:
    return ParseForExpr();
  case tok_var:
    return ParseVarExpr();
  case tok_string:
    return ParseStrExpr();
  case tok_char:
    return ParseCharExpr();
  case tok_addr:
    return ParseAddrExpr();
  case tok_deref:
    return ParseDerefExpr();
  case tok_sizeof:
    return ParseSizeofExpr();
  case tok_while:
    return ParseWhileExpr();
  case tok_loop:
    return ParseLoopExpr();
  case tok_typeid:
    return ParseTypeidExpr();
  case tok_null:
    return ParseNullExpr();
  case tok_cast:
    return ParseCastExpr();
  case tok_goto:
    return ParseGotoExpr();
  case tok_label:
    return ParseLabelExpr();
  case tok_break:
    return ParseBreakExpr();
  case tok_match:
    return ParseMatch();
  case tok_defer:
    return ParseDefer();
  case tok_func:
    return ParseClosure();
  case tok_vector:
    return ParseConstantVector();
  }
}

std::unique_ptr<ExprAST> ParseConstantVector(){
    Log::Info() << "PARSE CONSTANT VECTOR" << "\n";
    bool is_vector = false;
    Cpoint_Type vector_element_type;
    int vector_element_number = 0;
    if (ParseTypeDeclarationVector(is_vector, vector_element_type, vector_element_number) == 1){
        return nullptr;
    }
    std::vector<std::unique_ptr<ExprAST>> VectorMembers;
    int member_nb = 0;

    if (CurTok != '{'){
        return LogError("Missing '{' in Vector constant expr");
    }
    getNextToken();
    while (true){
        if (CurTok == '}'){
            getNextToken();
            break;
        }
        if (member_nb > 0){
            if (CurTok != ','){
            return LogError("missing \',\' in constant array");
            }
            getNextToken();
        }
        auto E = ParseExpression();
        if (!E){
            return nullptr;
        }
        VectorMembers.push_back(std::move(E));
        member_nb++;
    }
    return std::make_unique<ConstantVectorExprAST>(vector_element_type, vector_element_number, std::move(VectorMembers));
}

std::unique_ptr<ExprAST> ParseInlineAsm(){
    std::unique_ptr<StringExprAST> asm_code = get_Expr_from_ExprAST<StringExprAST>(ParseStrExpr());
    if (!asm_code){
        return nullptr;
    }
    std::vector<std::unique_ptr<ArgInlineAsm>> InputOutputArgs;
    if (CurTok == ','){
        getNextToken();
        while (1) {
            ArgInlineAsm::ArgType argType;
            if (IdentifierStr == "in"){
                argType = ArgInlineAsm::ArgType::input;
            } else if (IdentifierStr == "out") {
                argType = ArgInlineAsm::ArgType::output;
            } else {
                return LogError("Unknown type of arg for the asm macro");
            }
            getNextToken();
            auto argExpr = ParseExpression();
            InputOutputArgs.push_back(std::make_unique<ArgInlineAsm>(std::move(argExpr), argType));
            if (CurTok == ')'){
                break;
            }
            if (CurTok != ','){
                return LogError("Expected ')' or ',' in argument list of the asm macro");
            }
            getNextToken();
        }
    }
    getNextToken(); // eat ')'
    std::unique_ptr<ArgsInlineAsm> argsInlineAsm = std::make_unique<ArgsInlineAsm>(std::move(asm_code), std::move(InputOutputArgs));
    return generate_asm_macro(std::move(argsInlineAsm));
}

std::unique_ptr<ExprAST> ParseMacroCall(){
    std::vector<std::unique_ptr<ExprAST>> ArgsMacro;
    Log::Info() << "Parsing macro call" << "\n";
    getNextToken();
    std::string function_name = IdentifierStr;
    getNextToken();
    if (CurTok != '('){
        return LogError("missing '(' in the call of a macro function");
    }
    getNextToken();
    if (function_name == "asm"){
        return ParseInlineAsm();
    } else {
    auto ret = ParseFunctionArgs(ArgsMacro);
    if (!ret){
        return nullptr;
    }
    
    getNextToken();
    if (function_name == "file"){
        return get_filename_tok();
    } else if (function_name == "expect"){
        return generate_expect(ArgsMacro);
    } else if (function_name == "panic"){
        return generate_panic(ArgsMacro);
    } else if (function_name == "stringify"){
        if (ArgsMacro.size() != 1){
            return LogError("Wrong number of args for %s macro function call : expected %d, got %d", "stringify", 1, ArgsMacro.size());
        }
        return stringify_macro(ArgsMacro.at(0));
    } else if (function_name == "concat"){
        return generate_concat_macro(ArgsMacro);
    } else if (function_name == "line"){
        return std::make_unique<StringExprAST>(Comp_context->line);
    } else if (function_name == "line_nb"){
        return std::make_unique<NumberExprAST>((double)Comp_context->curloc.line_nb);
    } else if (function_name == "column_nb"){
        return std::make_unique<NumberExprAST>((double)Comp_context->curloc.col_nb);
    } else if (function_name == "time"){
        return generate_time_macro();
    } else if (function_name == "env"){
        return generate_env_macro(ArgsMacro);
    } else if (function_name == "arch"){
        return std::make_unique<StringExprAST>(context->get_variable_value("ARCH"));
    } else if (function_name == "todo") {
        return generate_todo_macro(ArgsMacro);
    } else if (function_name == "dbg"){
        return generate_dbg_macro(ArgsMacro);
    } else if (function_name == "print"){
        return generate_print_macro(ArgsMacro, false, false);
    } else if (function_name == "println"){
        return generate_print_macro(ArgsMacro, true, false);
    } else if (function_name == "eprint"){
        return generate_print_macro(ArgsMacro, false, true);
    } else if (function_name == "eprintln"){
        return generate_print_macro(ArgsMacro, true, true);
    } else if (function_name == "unreachable"){
        return generate_unreachable_macro();
    } else if (function_name == "assume"){
        return generate_assume_macro(ArgsMacro);
    }
    }
    return LogError("Unknown function macro called : %s", function_name.c_str());
}

std::unique_ptr<ExprAST> ParseSemiColon(){
    getNextToken();
    return std::make_unique<SemicolonExprAST>();
}

int ParseTypeDeclarationVector(bool& is_vector, Cpoint_Type& vector_element_type, int& vector_element_number){
    is_vector = true;
    getNextToken();
    
    if (CurTok != '~'){
        LogError("Missing opening '~' in Vector type declaration");
        return -1;
    }
    getNextToken();

    if (CurTok != tok_identifier || !is_type(IdentifierStr)){
        LogError("Missing type in Vector type");
        return -1;
    }
    vector_element_type = Cpoint_Type(get_type(IdentifierStr));
    getNextToken();
    if (CurTok != ','){
        LogError("Missing number of Vector element in type declaration");
        return -1;
    }
    getNextToken();

    auto number_expr = ParseNumberExpr();
    if (!number_expr){
        LogError("Missing number of Vector element in type declaration");
        return -1;
    }
    vector_element_number = (int)dynamic_cast<NumberExprAST*>(number_expr.get())->Val;

    if (CurTok != '~'){
        LogError("Missing closing '~' in Vector type declaration");
        return -1;
    }
    getNextToken();
    //Log::Info() << "token end of vector type parsing : " << CurTok << "\n";
    return 0;
}

Cpoint_Type ParseTypeDeclaration(bool eat_token /*= true*/, bool is_return /*= false*/, bool no_default_type /*=false*/){
  int type = double_type; 
  bool is_ptr = false;
  std::string struct_Name = "";
  std::string unionName = "";
  std::string enumName = "";
  int nb_ptr = 0;
  bool is_function = false;
  bool is_template_type = false;
  std::vector<Cpoint_Type> args;
  Cpoint_Type* return_type = nullptr;
  Cpoint_Type default_type = Cpoint_Type(double_type);
  if (no_default_type){
    default_type = Cpoint_Type();
  }
  bool is_object_template = false;
  Cpoint_Type* object_template_type_passed = nullptr;
  bool is_vector = false;
  int vector_element_number = 0;
  Cpoint_Type vector_element_type;
  if (eat_token){
  getNextToken(); // eat the ':'
  }
  if (CurTok == tok_never){
    if (!is_return){
        LogError("Can't use the never type not in a return");
        return Cpoint_Type();
    }
    getNextToken();
    return Cpoint_Type(never_type);
  }
  if (CurTok != tok_identifier && CurTok != tok_struct && CurTok != tok_class && CurTok != tok_func && CurTok != tok_union && CurTok != tok_enum && CurTok != tok_vector){
    LogError("expected identifier after var in type declaration");
    return default_type;
  }
  // TODO make a way to recognize a template type in cpoint_type. Remove the following code because the replacement of the type will be made in codegen
  if (CurTok == tok_identifier && IdentifierStr == TypeTemplateCallCodegen.first){
    Log::Info() << "Found template type in template call" << "\n";
    Cpoint_Type temp_type =  TypeTemplateCallCodegen.second;
    return temp_type;
  }
  if (CurTok == tok_func){
    is_function = true;
    getNextToken();
    if (CurTok != '('){
        LogError("Missing parenthesis");
        return default_type;
    }
    getNextToken();
    if (CurTok != ')'){
    while (1){
        if (CurTok != tok_identifier){
            LogError("Missing Identifier for args in function type");
            return default_type;
        }
        Cpoint_Type arg_type = ParseTypeDeclaration(false);
        args.push_back(arg_type);
        if (CurTok == ')'){
            Log::Info() << "Found closing )" << "\n";
            getNextToken();
            break;
        }
        if (CurTok != ','){
            LogError("Missing ',' in args types for function type");
            return default_type;
        }
        getNextToken();
    }
    }
    Cpoint_Type return_type_temp = ParseTypeDeclaration(false);
    return_type = new Cpoint_Type(return_type_temp);
    goto before_gen_cpoint_type;
  }
  if (CurTok == tok_vector){
    if (ParseTypeDeclarationVector(is_vector, vector_element_type, vector_element_number) == -1){
        return default_type;
    }
  } else if (CurTok == tok_struct || CurTok == tok_class){
    getNextToken();
    struct_Name = IdentifierStr;
    type = other_type;
    Log::Info() << "struct_Name " << struct_Name << "\n";
    getNextToken();
    if (CurTok == '~'){
        Log::Info() << "found templates for struct" << "\n";
        getNextToken();
        object_template_type_passed = new Cpoint_Type(ParseTypeDeclaration(false, false, true));
        if (!object_template_type_passed){
            LogError("missing template type for struct");
            return default_type;
        }
        if (CurTok != '~'){
            LogError("Missing '~' in struct template type usage");
            return default_type;
        }
        getNextToken();
        auto structDeclar = TemplateStructDeclars[struct_Name]->declarAST->clone();
        structDeclar->Name = get_object_template_name(struct_Name, *object_template_type_passed);
        StructTemplatesToGenerate.push_back(std::make_unique<TemplateStructCreation>(*object_template_type_passed, std::move(structDeclar)));
        Log::Info() << "added to StructTemplatesToGenerate" << "\n";
        is_object_template = true;
    }
    while (CurTok == tok_ptr){
      is_ptr = true;
      nb_ptr++;
      Log::Info() << "Type Declaration ptr added : " << nb_ptr << "\n";
      getNextToken();
    }
  } else if (CurTok == tok_union){
    type = other_type;
    getNextToken();
    unionName = IdentifierStr;
    getNextToken();
    if (CurTok == tok_ptr){
      is_ptr = true;
      getNextToken();
    }
  } else if (CurTok == tok_enum){
    type = other_type;
    getNextToken();
    enumName = IdentifierStr;
    getNextToken();
    if (CurTok == '~'){
        getNextToken();
        object_template_type_passed = new Cpoint_Type(ParseTypeDeclaration(false, false, true));
        if (!object_template_type_passed){
            LogError("missing template type for enum");
            return default_type;
        }
        if (CurTok != '~'){
            LogError("Missing '~' in enum template type usage");
            return default_type;
        }
        getNextToken();
        auto enumDeclar = TemplateEnumDeclars[enumName]->declarAST->clone();
        enumDeclar->Name = get_object_template_name(enumName, *object_template_type_passed);
        EnumTemplatesToGenerate.push_back(std::make_unique<TemplateEnumCreation>(*object_template_type_passed, std::move(enumDeclar)));
        Log::Info() << "added to EnumTemplatesToGenerate" << "\n";
        is_object_template = true;
    }
  } else if (is_type(IdentifierStr)){
    type = get_type(IdentifierStr);
    Log::Info() << "Variable type : " << type << "\n";
    getNextToken();
    while (CurTok == tok_ptr){
      is_ptr = true;
      nb_ptr++;
      Log::Info() << "Type Declaration ptr added : " << nb_ptr << "\n";
      getNextToken();
    }
  } else if (TypeTemplateCallAst == IdentifierStr){
    Log::Info() << "Template type in parsing type declaration" << "\n";
    is_template_type = true;
    getNextToken();
    while (CurTok == tok_ptr){
      is_ptr = true;
      nb_ptr++;
      Log::Info() << "Type Declaration ptr added : " << nb_ptr << "\n";
      getNextToken();
    }
  } else {
    Log::Info() << "TypeTemplatCallAst : " << TypeTemplateCallAst << "\n";
    LogError("Wrong type %s found", IdentifierStr.c_str());
    return default_type;
  }
before_gen_cpoint_type:
  return Cpoint_Type(type, is_ptr, nb_ptr, false, 0, struct_Name != "", struct_Name, unionName != "", unionName, enumName != "", enumName, is_template_type, is_object_template, object_template_type_passed, is_function, args, return_type, is_vector, new Cpoint_Type(vector_element_type), vector_element_number);
}

std::unique_ptr<ExprAST> ParseFunctionArgs(std::vector<std::unique_ptr<ExprAST>>& Args){
  Log::Info() << "CurTok when parsing function args : " << CurTok << "\n";
  if (CurTok != ')') {
    while (1) {
      if (auto Arg = ParseExpression()){
        Args.push_back(std::move(Arg));
        Log::Info() << "adding arg" << "\n";
      } else {
        return nullptr;
      }
      if (CurTok == ')')
        break;

      if (CurTok != ','){
        Log::Info() << "CurTok : " << CurTok << "\n";
        Log::Info() << "OpStringMultiChar : " << OpStringMultiChar << "\n";
        return LogError("Expected ')' or ',' in argument list");
      }
      getNextToken();
    }
  }
  return std::make_unique<EmptyExprAST>();
}

std::unique_ptr<ExprAST> ParseFunctionArgsTyped(std::vector<std::pair<std::string, Cpoint_Type>>& ArgNames, bool& is_variable_number_args){
    if (CurTok != ')'){
    int arg_nb = 0;
    while (1){
      if (CurTok == tok_format){
      Log::Info() << "Found Variable number of args" << "\n";
      is_variable_number_args = true;
      getNextToken();
      break;
      } else {
      if (CurTok == ')'){
        break;
      }
      if (arg_nb > 0){
        if (CurTok != ','){
          Log::Info() << "CurTok : " << CurTok << "\n";
          return LogError("Expected ')' or ',' in argument list");
        }
        getNextToken();
      }
      std::string ArgName = IdentifierStr;
      getNextToken();
      Cpoint_Type arg_type = Cpoint_Type(double_type);
      if (CurTok == ':'){
        arg_type = ParseTypeDeclaration();
      }
      arg_nb++;
      ArgNames.push_back(std::make_pair(ArgName, arg_type));
      }
    }
  }
  return std::make_unique<EmptyExprAST>();
}

static bool is_infinite_loop(std::unique_ptr<ExprAST>& E){
  if (dynamic_cast<LoopExprAST*>(E.get())){
    auto loopExpr = dynamic_cast<LoopExprAST*>(E.get());
    if (loopExpr->is_infinite_loop){
      return true;
    }
  }
  // TODO : add verification for for and while loops in cases where the expression is always true (add better constness analysis for cases like 2 == 2)
  if (dynamic_cast<WhileExprAST*>(E.get())){
    // TODO : also need to verify no exprs contains breaks 
    /*auto WhileExpr = dynamic_cast<WhileExprAST*>(E.get());
    return dynamic_cast<BoolExprAST*>(WhileExpr->Cond.get()) && dynamic_cast<BoolExprAST*>(WhileExpr->Cond.get())->val;*/
  }
  return false;
}


std::unique_ptr<ExprAST> ParseBodyExpressions(std::vector<std::unique_ptr<ExprAST>>& Body, bool is_func_body){
    //bool return_found = false;
    bool found_terminator = false;
    //bool infinite_loop_found = false;
    while (CurTok != '}'){
      auto E = ParseExpression();
      if (!E)
        return nullptr;
      if (found_terminator /*return_found ||  infinite_loop_found*/){
        //auto unreachable_code_warning = Log::Warning();
        //unreachable_code_warning.content << "unreachable code : " << E->to_string();
        continue;
      }
      if (is_infinite_loop(E) || E->contains_expr(ExprType::Return) || E->contains_expr(ExprType::Unreachable)){
        if (!found_terminator){
          auto unreachable_code_warning = Log::Warning(E->loc);
          std::string terminator_name = "expr";
          unreachable_code_warning.head << "Unreachable code found\n";
          unreachable_code_warning.content << "Any statement after this line is not reachable\n";
          unreachable_code_warning.end();
        }
        found_terminator = true;
        
      }
      /*if (dynamic_cast<LoopExprAST*>(E.get())){
        auto loopExpr = dynamic_cast<LoopExprAST*>(E.get());
        if (loopExpr->is_infinite_loop){
          //infinite_loop_found = true;
          should_break = true;
        }
      }*/
      /*if (is_func_body){ // TODO : is this if necessary
        if (dynamic_cast<ReturnAST*>(E.get())){
            return_found = true;
        }
      }*/
      Body.push_back(std::move(E));
    }
    // if the last expressions is a return, replace it with just the expressions because otherwise it would return a never type 
    if (is_func_body && dynamic_cast<ReturnAST*>(Body.back().get())){
        auto return_expr = std::move(Body.back());
        Body.pop_back();
        Body.push_back(std::move(dynamic_cast<ReturnAST*>(return_expr.get())->returned_expr));
    }
    return std::make_unique<EmptyExprAST>(); 
}

std::unique_ptr<ExprAST> ParseBool(bool bool_value){
  Log::Info() << "Parsing bool" << "\n";
  getNextToken();
  return std::make_unique<BoolExprAST>(bool_value);
}

bool struct_parsing = false;

static std::unique_ptr<PrototypeAST> ParsePrototype() {
  std::string FnName;
  Source_location FnLoc = Comp_context->curloc;
  Log::Info() << "FnLoc : " << Comp_context->curloc << "\n";
  bool is_variable_number_args = false;
  unsigned Kind = 0;  // 0 = identifier, 1 = unary, 2 = binary.
  unsigned BinaryPrecedence = 30;
  bool has_template = false;
  std::string template_name = "";
  switch (CurTok){
    default:
      return LogErrorP("Expected function name in prototype");
    case tok_identifier:
      FnName = IdentifierStr;
      Kind = 0;
      getNextToken();
      break;
    case tok_unary:
      getNextToken();
      if (!isascii(CurTok))
        return LogErrorP("Expected unary operator");
      FnName = "unary";
      FnName += (char)CurTok;
      Kind = 1;
      getNextToken();
      break;
    case tok_binary:
      getNextToken();
      if (!isascii(CurTok))
        return LogErrorP("Expected binary operator");
      FnName = "binary";
      FnName += (char)CurTok;
      Kind = 2;
      getNextToken();
      // Read the precedence if present.
      if (CurTok == tok_number) {
      if (NumVal < 1 || NumVal > 100)
        return LogErrorP("Invalid precedence: must be 1..100");
      BinaryPrecedence = (unsigned)NumVal;
      getNextToken();
      }
      break;
  }
  // find template/generic
  if (CurTok == '~'){
    getNextToken();
    template_name = IdentifierStr;
    getNextToken();
    if (CurTok != '~'){
      return LogErrorP("Missing '~' in template declaration");
    }
    getNextToken();
    has_template = true;
    Log::Info() << "Found template" << "\n";
    TypeTemplateCallAst = template_name;
  }
  if (CurTok != '(')
    return LogErrorP("Expected '(' in prototype");

  // Read the list of argument names.
  std::vector<std::pair<std::string,Cpoint_Type>> ArgNames;
  if (FnName == "main"){
    getNextToken();
    while (CurTok == tok_identifier || CurTok == ':'){
      getNextToken();
    }
    ArgNames.push_back(std::make_pair("argc", Cpoint_Type(i32_type)));
    ArgNames.push_back(std::make_pair("argv",  Cpoint_Type(i8_type, true, 2)));
  } else {
  getNextToken();
  auto ret = ParseFunctionArgsTyped(ArgNames, is_variable_number_args);
  if (!ret){
    return nullptr;
  }
  }

  // success.
  getNextToken();  // eat ')'.
  if (Kind && ArgNames.size() != Kind)
    return LogErrorP("Invalid number of operands for operator : %d args but %d expected", ArgNames.size(), Kind);
  Log::Info() << "Tok : " << CurTok << "\n";
  Cpoint_Type cpoint_type = Cpoint_Type(double_type);
  if (CurTok == tok_identifier || CurTok == tok_struct || CurTok == tok_class || CurTok == tok_never){
    Log::Info() << "Tok type : " << CurTok << "\n";
    Log::Info() << "type : " << IdentifierStr << "\n";
    cpoint_type = ParseTypeDeclaration(false, true);
  }
  /*if (!modulesNamesContext.empty()){
    std::string module_mangled_function_name = FnName;
    for (int i = modulesNamesContext.size()-1; i >= 0; i--){
      module_mangled_function_name = module_mangling(modulesNamesContext.at(i), module_mangled_function_name);
    }
    FnName = module_mangled_function_name;
    Log::Info() << "mangled name is : " << FnName << "\n";
  }*/
  Log::Info() << "FnLoc in ast : " << FnLoc << "\n";
  auto Proto = std::make_unique<PrototypeAST>(FnLoc, FnName, ArgNames, cpoint_type, Kind != 0, BinaryPrecedence, is_variable_number_args, has_template, template_name);
  /*if (!struct_parsing){
  FunctionProtos[FnName] = Proto->clone();
  }*/
  return Proto;
}

static std::unique_ptr<VarExprAST> find_in_vector_of_unique_ptr_varexpr(std::vector<std::unique_ptr<VarExprAST>>& vec, std::string var_to_find){
    for (int i = 0; i < vec.size(); i++){
        // TODO : maybe iterate over all vars in VarNames
        std::string var_name = vec.at(i)->VarNames.at(0).first;
        if (var_name == var_to_find){
            return get_Expr_from_ExprAST<VarExprAST>(vec.at(i)->clone());
        }
    }
    return nullptr;
}

std::string struct_parsing_template_name = "";

std::unique_ptr<StructDeclarAST> ParseStruct(){
  struct_parsing = true;
  std::string template_name = "";
  bool has_template = false;
  std::vector<std::unique_ptr<VarExprAST>> VarList;
  std::vector<std::unique_ptr<FunctionAST>> Functions;
  std::vector<std::unique_ptr<PrototypeAST>> ExternFunctions;
  getNextToken(); // eat struct
  std::string structName = IdentifierStr;
  Log::Info() << "Struct Name : " << structName << "\n";
  std::string struct_repr = "default";
  getNextToken();
  if (CurTok == '~'){
    getNextToken();
    template_name = IdentifierStr;
    getNextToken();
    if (CurTok != '~'){
      struct_parsing = false;
      return LogErrorS("Missing '~' in template struct declaration");
    }
    getNextToken();
    has_template = true;
    TypeTemplateCallAst = template_name;
  }
  struct_parsing_template_name = template_name;
  if (CurTok == tok_identifier && IdentifierStr == "repr"){
    getNextToken();
    if (CurTok != tok_string){
        struct_parsing = false;
        return LogErrorS("Expected a repr name (as a string) after 'repr' in Struct");
    }
    struct_repr = strStatic;
    getNextToken();
  }
  if (CurTok != '{'){
    struct_parsing = false;
    return LogErrorS("Expected '{' in Struct");
  }
  getNextToken();

  StructDeclarations[structName] = std::make_unique<StructDeclaration>(nullptr, nullptr, std::vector<std::pair<std::string,Cpoint_Type>>(), std::vector<std::string>());
  while ((CurTok == tok_var || CurTok == tok_func || CurTok == tok_extern) && CurTok != '}'){
    Log::Info() << "Curtok in struct parsing : " << CurTok << "\n";
    if (CurTok == tok_var){
        auto exprAST = ParseVarExpr();
        if (!exprAST){
            struct_parsing = false;
            return nullptr;
        }
        std::unique_ptr<VarExprAST> declar = get_Expr_from_ExprAST<VarExprAST>(std::move(exprAST));
        if (!declar){
            struct_parsing = false;
            return LogErrorS("Error in struct declaration vars");
        }
        std::string VarName = declar->VarNames.at(0).first;
        StructDeclarations[structName]->members.push_back(std::make_pair(VarName, declar->cpoint_type));
        VarList.push_back(std::move(declar));
    } else if (CurTok == tok_extern){
        getNextToken();
        std::unique_ptr<PrototypeAST> Proto = ParsePrototype();
        if (Proto == nullptr){ return nullptr; }
        std::string function_name;
        if (Proto->Name == structName){
            // Constructor
            function_name = "Constructor__Default";
        } else {
            function_name = Proto->Name;
        }
        StructDeclarations[structName]->functions.push_back(function_name);
        ExternFunctions.push_back(std::move(Proto));
        Log::Info() << "CurTok after extern : " << CurTok << "\n";
        
    } else if (CurTok == tok_func){
      Log::Info() << "function found in struct" << "\n";
      Cpoint_Type self_type = Cpoint_Type(other_type, true, 1, false, 0, true, structName);
      NamedValues["self"] = std::make_unique<NamedValue>(nullptr, self_type);
      std::unique_ptr<FunctionAST> declar = ParseDefinition();
      Log::Info() << "AFTER ParseDefinition" << "\n";
      if (!declar){
        return nullptr;
      }
      std::string function_name;
      //std::cout << structName << "\n";
      // TODO : maybe remove this because it is done in codegen
      if (declar->Proto->Name == structName){
        // Constructor
        function_name = "Constructor__Default";
      } else {
        function_name = declar->Proto->Name;
      }
      StructDeclarations[structName]->functions.push_back(function_name);
      Functions.push_back(std::move(declar));
    } else {
      return LogErrorS("Unexpected expression in struct declaration");
    }
  }
  Log::Info() << "CurTok : " << CurTok << "\n";
  if (CurTok != '}'){
    struct_parsing = false;
    return LogErrorS("Expected '}' in struct");
  }
  getNextToken();  // eat '}'.
  if (!has_template){
    std::vector<std::pair<std::string, Cpoint_Type>> members_reordered;
    bool is_reordering_needed = reorder_struct(StructDeclarations[structName].get(), structName, members_reordered);
    if (is_reordering_needed){
    if (struct_repr == "auto_reorder"){
        StructDeclarations[structName]->members = members_reordered;
        std::vector<std::unique_ptr<VarExprAST>> VarsReordered;
        for (int i = 0; i < StructDeclarations[structName]->members.size(); i++){
            std::string var_name_temp = StructDeclarations[structName]->members.at(i).first;
            std::unique_ptr<VarExprAST> temp_var_expr = find_in_vector_of_unique_ptr_varexpr(VarList, var_name_temp);
            assert(temp_var_expr != nullptr);
            VarsReordered.push_back(std::move(temp_var_expr));
        }
        VarList = std::move(VarsReordered);
    } else if (struct_repr != "C"){
        // TODO : add warning (add a Big warning to have the comparison between the new and the old order)
        // TODO : move this warning in a separate function (and in another file ?)
        std::string old_struct = "\tstruct " + structName + " {\n";
        for (int i = 0; i < StructDeclarations[structName]->members.size(); i++){
            old_struct += "\t\tvar " + StructDeclarations[structName]->members.at(i).first + " : " + create_pretty_name_for_type(StructDeclarations[structName]->members.at(i).second) + "\n";
        }
        old_struct += "\t}";
        std::string new_struct = "\tstruct " + structName + " {\n";
        for (int i = 0; i < members_reordered.size(); i++){
            new_struct += "\t\tvar " + members_reordered.at(i).first + " : " + create_pretty_name_for_type(members_reordered.at(i).second) + "\n";
        }
        new_struct += "\t}";
        auto reorder_struct_warning = Log::Warning();
        reorder_struct_warning.head << "The " << structName << " fields could be reordered to optimize its size : \n";
        reorder_struct_warning.content << "Current struct : \n" << old_struct << "\n"
                                       << "Reordered struct : \n" << new_struct << "\n";
        reorder_struct_warning.end();

    }
    }
  }
  auto structDeclar = std::make_unique<StructDeclarAST>(structName, std::move(VarList), std::move(Functions), std::move(ExternFunctions), has_template, template_name);
  struct_parsing = false;
  struct_parsing_template_name = "";
  if (has_template){
    Log::Info() << "Parse struct has template" << "\n";
    TemplateStructDeclars[structName] = std::make_unique<StructDeclar>(std::move(structDeclar), template_name);
    is_template_parsing_struct = true;
    return nullptr;
  }
  return structDeclar;
}

std::unique_ptr<MembersDeclarAST> ParseMembers(){
    std::string members_name = "";
    std::string members_for = "";
    std::vector<std::unique_ptr<FunctionAST>> Functions;
    std::vector<std::unique_ptr<PrototypeAST>> Externs;
    getNextToken(); // eat members
    if (CurTok == tok_identifier){
        members_name = IdentifierStr;
        getNextToken();
    }
    if (CurTok != tok_for){
        return LogErrorMembers("Expected Identifier or for token in members block");
    }
    getNextToken();
    members_for = IdentifierStr;
    getNextToken();
    if (CurTok != '{')
        return LogErrorMembers("Expected '{' in Struct");
    getNextToken();
    while ((CurTok == tok_func || CurTok == tok_extern) && CurTok != '}'){
        if (CurTok == tok_func){
            Cpoint_Type self_type;
            if (!is_type(members_for)){
                self_type = Cpoint_Type(other_type, true, 0, false, 0, true, members_for);;
            } else {
                self_type = Cpoint_Type(get_type(members_for));
            }
            NamedValues["self"] = std::make_unique<NamedValue>(nullptr, self_type);
            auto funcAST = ParseDefinition();
            if (funcAST == nullptr){
                return LogErrorMembers("Error in members declaration funcs");
            }
            Functions.push_back(std::move(funcAST));
        } else if (CurTok == tok_extern){
            auto externAST = ParseExtern();
            if (externAST == nullptr){
                return LogErrorMembers("Error in members extern funcs");
            }
            Externs.push_back(std::move(externAST));
        } else {
            return LogErrorMembers("Unknown expression in members block");
        }
        if (CurTok == ';'){
            getNextToken();
        }
    }
    Log::Info() << "CurTok : " << CurTok << "\n";
    if (CurTok != '}'){
        return LogErrorMembers("Expected '}' in members");
    }
    getNextToken();  // eat '}'.
    return std::make_unique<MembersDeclarAST>(members_name, members_for, std::move(Functions), std::move(Externs));
}

std::unique_ptr<UnionDeclarAST> ParseUnion(){
  Log::Info() << "Parsing Union" << "\n";
  getNextToken(); // eat union
  std::vector<std::unique_ptr<VarExprAST>> VarList;
  std::string unionName = IdentifierStr;
  getNextToken();
  if (CurTok != '{')
    return LogErrorU("Expected '{' in Union");
  getNextToken();
  while (CurTok == tok_var && CurTok != '}'){
    auto exprAST = ParseVarExpr();
    VarExprAST* varExprAST = dynamic_cast<VarExprAST*>(exprAST.get());
    if (varExprAST == nullptr){
      return LogErrorU("Error in union declaration vars");
    }
    std::unique_ptr<VarExprAST> declar;
    exprAST.release();
    declar.reset(varExprAST);
    if (!declar){return nullptr;}
    VarList.push_back(std::move(declar));
  }
  if (CurTok != '}'){
    return LogErrorU("Expected '}' in Union");
  }
  getNextToken();  // eat '}'.
  return std::make_unique<UnionDeclarAST>(unionName, std::move(VarList));
}

std::unique_ptr<EnumDeclarAST> ParseEnum(){
    getNextToken(); // eat enum
    std::string Name;
    bool enum_member_contain_type = false;
    std::vector<std::unique_ptr<EnumMember>> EnumMembers;
    Name = IdentifierStr;
    getNextToken();
    bool has_template = false;
    std::string template_name = "";
    if (CurTok == '~'){
        has_template = true;
        getNextToken();
        if (CurTok != tok_identifier){
            return LogErrorE("Missing template type name in enum declaration");
        }
        template_name = IdentifierStr;
        TypeTemplateCallAst = template_name;
        getNextToken();

        if (CurTok != '~'){
            return LogErrorE("Missing '~' in enum declaration");
        }
        getNextToken();
    }
    if (CurTok != '{'){
        return LogErrorE("Expected '{' in Enum");
    }
    getNextToken();
    while (CurTok == tok_identifier && CurTok != '}'){
        std::unique_ptr<EnumMember> enumMember;
        bool contains_value = false;
        bool contains_custom_index = false;
        std::unique_ptr<Cpoint_Type> Type = nullptr;
        int Index = -1;
        std::string memberName = IdentifierStr;
        getNextToken();
        if (CurTok == '('){
            getNextToken();
            contains_value = true;
            enum_member_contain_type = true;
            Type = std::make_unique<Cpoint_Type>(ParseTypeDeclaration(false));
            if (CurTok != ')'){
                return LogErrorE("Expected ')' after '(' in type declaration of Enum member");
            }
            getNextToken();
        }
        if (CurTok == '='){
            getNextToken();
            contains_custom_index = true;
            auto IndexTemp = ParseExpression();
            auto indexCodegened = IndexTemp->codegen();
            if (!dyn_cast<Constant>(indexCodegened)){
                return LogErrorE("Index in Declaration of enum is not a constant");
            }
            Index = from_val_to_int(indexCodegened);
            Log::Info() << "EnumDeclarAST parsing custom Index : " << Index << " " << contains_custom_index << "\n";
        }
        Log::Info() << "EnumDeclarAST contains_custom_index : " << contains_custom_index << "\n";
        EnumMembers.push_back(std::make_unique<EnumMember>(memberName, contains_value, std::move(Type), contains_custom_index, Index));
    }
    if (CurTok != '}'){
        return LogErrorE("Expected '}' in Enum");
    }
    getNextToken();
    auto enumDeclar = std::make_unique<EnumDeclarAST>(Name, enum_member_contain_type, std::move(EnumMembers), has_template, template_name);
    if (has_template){
        TemplateEnumDeclars[Name] = std::make_unique<EnumDeclar>(std::move(enumDeclar), template_name);
        is_template_parsing_enum = true;
        return nullptr;
    }
    return enumDeclar;
}

std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken();  // eat func.
  auto Proto = ParsePrototype();
  if (!Proto) return nullptr;
  if (CurTok != '{'){
    LogErrorF("Expected '{' in function definition");
  }
  getNextToken();  // eat '{'
  for (int i = 0; i < Proto->Args.size(); i++){
    Log::Info() << "args added to NamedValues when doing ast : " << Proto->Args.at(i).first << "\n";
    NamedValues[Proto->Args.at(i).first] = std::make_unique<NamedValue>(nullptr,Proto->Args.at(i).second);
  }
  StructTemplatesToGenerate.clear();
  Log::Info() << "cleared StructTemplatesToGenerate" << "\n";
  std::vector<std::unique_ptr<ExprAST>> Body;
  if (Comp_context->std_mode && Proto->Name == "main" && Comp_context->gc_mode){
  generate_gc_init(Body);
  }
  auto ret = ParseBodyExpressions(Body, true);
  if (!ret){
    return nullptr;
  }
  getNextToken(); // eat }
  Log::Info() << "end of function" << "\n";
  bool has_template = Proto->has_template;
  std::string FunctionName = Proto->Name;
  std::unique_ptr<FunctionAST> functionAST = std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
  if (has_template){
    std::string template_name = functionAST->Proto->template_name;
    Log::Info() << "Added template : " << template_name << "\n";
    TemplateProtos[FunctionName] = std::make_unique<TemplateProto>(std::move(functionAST), template_name);
    is_template_parsing_definition = true;
    return nullptr; // no generation of function because it is a template and will depend on the type. Change to other value because nullptr causes to skip token
  }
  return functionAST;
}

std::unique_ptr<PrototypeAST> ParseExtern() {
  getNextToken();  // eat extern.
  return ParsePrototype();
}


std::unique_ptr<ExprAST> ParseStrExpr(){
  Log::Info() << "ParseStrExpr " << strStatic << "\n";
  auto string = std::make_unique<StringExprAST>(strStatic);
  Log::Info() << "Before getNextToken" << "\n";
  getNextToken();
  return string;
}

std::unique_ptr<ExprAST> ParseCharExpr(){
  Log::Info() << "ParseChar : " << charStatic << "\n";
  auto charAST = std::make_unique<CharExprAST>(charStatic);
  getNextToken();
  return charAST;
}

std::unique_ptr<ExprAST> ParseAddrExpr(){
  getNextToken();  // eat addr.
  std::unique_ptr<ExprAST> exprAddr;
    exprAddr = ParseExpression();
  if (!exprAddr){
    return nullptr;
  }
  return std::make_unique<AddrExprAST>(std::move(exprAddr));
}

std::unique_ptr<ExprAST> ParseDerefExpr(){
    Log::Info() << "parsing deref" << "\n";
    getNextToken();  // eat deref.
    std::unique_ptr<ExprAST> exprDeref = ParseExpression();
    if (!exprDeref){
        return nullptr;
    }
    return std::make_unique<DerefExprAST>(std::move(exprDeref));
}

std::unique_ptr<ExprAST> ParseSizeofExpr(){
  Log::Info() << "Parsing sizeof" << "\n";
  getNextToken();
  Cpoint_Type type;
  std::unique_ptr<ExprAST> expr = nullptr;
  bool is_sizeof_type = false;
  // move this to is_type_declaration
  if ((CurTok == tok_identifier && (is_type(IdentifierStr) || struct_parsing_template_name == IdentifierStr)) || CurTok == tok_struct || CurTok == tok_class){
    is_sizeof_type = true;
    type = ParseTypeDeclaration(false);
  } else {
    expr = ParseExpression();
    if (!expr){
        return nullptr;
    }
  }
  /*bool is_variable = false;
  std::string Name = "";
  if (NamedValues[IdentifierStr] != nullptr){
    Log::Info() << "sizeof is variable" << "\n";
    is_variable = true;
    Name = IdentifierStr;
    getNextToken();
  } else if ((CurTok == tok_identifier && is_type(IdentifierStr)) || CurTok == tok_struct || CurTok == tok_class){
    Log::Info() << "sizeof is type" << "\n";
    type = ParseTypeDeclaration(false);
    Log::Info() << "CurTok : " << CurTok << "\n";
    Log::Info() << "sizeof type parsed : " << type << "\n";
  } else {
    return LogError("Neither a type or a variable in sizeof expr");
  }*/
  auto Sizeof = std::make_unique<SizeofExprAST>(is_sizeof_type, type, std::move(expr));
  //Log::Info() << "after sizeof : " << Name << "\n";
  //getNextToken();
  return Sizeof;
}

std::unique_ptr<ExprAST> ParseCastExpr(){
    getNextToken(); // eat "cast"
    Cpoint_Type type = ParseTypeDeclaration(false);
    auto E = ParseExpression();
    return std::make_unique<CastExprAST>(std::move(E), type);
}

std::unique_ptr<ExprAST> ParseNullExpr(){
    getNextToken();
    return std::make_unique<NullExprAST>();
}

std::unique_ptr<TestAST> ParseTest(){
  getNextToken(); // eat "test" string
  if (CurTok != tok_string){
    return LogErrorT("Expected a string to describe the test");    
  }
  std::string description = strStatic;
  getNextToken();
  if (CurTok != '{'){
    return LogErrorT("Expected { in test");
  }
  getNextToken();
  std::vector<std::unique_ptr<ExprAST>> Body;
  auto ret = ParseBodyExpressions(Body);
  if (!ret){
    return nullptr;
  }
  getNextToken(); // eat '}'
  return std::make_unique<TestAST>(description, std::move(Body));
}

std::unique_ptr<ExprAST> ParseTypeidExpr(){
  getNextToken();
  auto expr = ParseExpression();
  if (!expr){
    return nullptr;
  }
  return std::make_unique<TypeidExprAST>(std::move(expr));
}

static bool is_templated_enum_creation(){
    //std::cout << "Curtok : " << CurTok << std::endl;
    std::flush(std::cout);
    if (CurTok != '~'){
        return false;
    }
    int i = 0;
    int peekChar = peekCharLine(i);
    std::cout << "peekChar start : " << peekChar << std::endl;
    while (peekChar != '\0' && peekChar != '('){
        peekChar = peekCharLine(i);
         std::cout << "peekChar at " << i << " : " << peekChar << std::endl;
        if (peekChar == ':'){
            return true;
        }
        i++;
    }
    return false;
}

std::unique_ptr<ExprAST> ParseUnary() {
  Log::Info() << "PARSE UNARY CurTok : " << CurTok << "\n";
  // If the current token is not an operator, it must be a primary expr.
  if (!isascii(CurTok) || is_templated_enum_creation() || CurTok == '(' || CurTok == ',' || CurTok == '{' || CurTok == ':' || CurTok == tok_string || CurTok == tok_false || CurTok == tok_true || CurTok == '[' || CurTok == '#' || CurTok == ';')
    return ParsePrimary();

  // If this is a unary operator, read it.
  Log::Info() << "Is an unary operator ? " << CurTok << "\n";
  int Opc = CurTok;
  getNextToken();
  if (auto Operand = ParseUnary())
    return std::make_unique<UnaryExprAST>(Opc, std::move(Operand));
  Log::Info() << "Not an unary operator. Curtok : " << CurTok << "\n";
  return nullptr;
}

std::unique_ptr<TypeDefAST> ParseTypeDef(){
  getNextToken(); // eat the 'type'
  std::string new_type = IdentifierStr;
  getNextToken();
  Cpoint_Type value_type = ParseTypeDeclaration(false);
  types_list.push_back(new_type);
  typeDefTable.push_back(value_type);
  return std::make_unique<TypeDefAST>(new_type, value_type);
}


extern bool debug_info_mode;

std::unique_ptr<ModAST> ParseMod(){
  std::vector<std::unique_ptr<FunctionAST>> functions;
  std::vector<std::unique_ptr<PrototypeAST>> function_protos;
  std::vector<std::unique_ptr<StructDeclarAST>> structs;
  std::vector<std::unique_ptr<ModAST>> mods;
  getNextToken();
  std::string mod_name = IdentifierStr;
  getNextToken();
  if (CurTok != '{'){
    return LogErrorM("missing '{' in mod");
  }
  getNextToken();
  //modulesNamesContext.push_back(mod_name);
  if (debug_info_mode){
    debugInfoCreateNamespace(mod_name);
  }
  while (CurTok != '}'){
    if (CurTok == tok_single_line_comment){
      HandleComment();
    } else if (CurTok == ';'){
      getNextToken();
    } else if (CurTok == tok_func){
      std::unique_ptr<FunctionAST> function = ParseDefinition();
      if (!function){
        return nullptr;
      }
      functions.push_back(std::move(function));
    } else if (CurTok == tok_extern){
      std::unique_ptr<PrototypeAST> function_proto = ParseExtern();
      if (!function_proto){
        return nullptr;
      }
      function_protos.push_back(std::move(function_proto));
    } else if (CurTok == tok_struct){
      std::unique_ptr<StructDeclarAST> structDeclar = ParseStruct();
      if (!structDeclar){
        return nullptr;
      }
      structs.push_back(std::move(structDeclar));
    } else if (CurTok == tok_mod){
      std::unique_ptr<ModAST> mod = ParseMod();
      if (!mod){
        return nullptr;
      }
      mods.push_back(std::move(mod));
    } else {
      return LogErrorM("Unexpected tok %d in mod declaration", CurTok);
    }
  }
  getNextToken(); //  eat }
  return std::make_unique<ModAST>(mod_name, std::move(functions), std::move(function_protos), std::move(structs), std::move(mods));
}

std::unique_ptr<GlobalVariableAST> ParseGlobalVariable(){
  getNextToken(); // eat the var.
  Cpoint_Type cpoint_type = Cpoint_Type(double_type);
  bool is_const = false;
  bool is_extern = false;
  if (IdentifierStr == "extern"){
    is_extern = true;
    getNextToken();
  }
  if (IdentifierStr == "const"){
    is_const = true;
    getNextToken();
  }
  std::string Name = IdentifierStr;
  getNextToken(); // eat identifier.
  std::unique_ptr<ExprAST> Index;
  bool is_array = false;
  if (CurTok == '['){
    is_array = true;
    getNextToken();
    Index = ParseNumberExpr();
    if (CurTok != ']'){
        return LogErrorG("missing ']'");
    }
    getNextToken();
  }
  if (CurTok == ':'){
    cpoint_type = ParseTypeDeclaration();
  }
  std::unique_ptr<ExprAST> Init = nullptr;
  if (CurTok == '=') {
  getNextToken(); // eat the '='.
  Init = ParseExpression();
  if (!Init)
    return nullptr;
  }
  std::string section_name = "";
  if (CurTok == tok_identifier && IdentifierStr == "section"){
    getNextToken();
    auto str_expr = ParseStrExpr();
    auto section_name_expression = static_cast<StringExprAST*>(str_expr.get());
    section_name = section_name_expression->str;
  }
  return std::make_unique<GlobalVariableAST>(Name, is_const, is_extern, cpoint_type, std::move(Init), is_array, std::move(Index), section_name);
}

std::unique_ptr<ExprAST> ParseIfExpr() {
  Source_location IfLoc = Comp_context->curloc;
  getNextToken();  // eat the if.

  // condition.
  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;
  std::unique_ptr<ExprAST> Then = ParseExpression();
  if (!Then){
    return nullptr;
  }
  std::unique_ptr<ExprAST> Else = nullptr;
else_start:
  if (CurTok == tok_else){
  getNextToken();
  if (CurTok == tok_if){
    auto else_if_expr = ParseIfExpr();
    if (!else_if_expr){
      return nullptr;
    }
    if (Else){
        Else = std::make_unique<ScopeExprAST>(make_unique_ptr_static_vector<ExprAST>(std::move(Else), std::move(else_if_expr)));
    } else {
        Else = std::make_unique<ScopeExprAST>(make_unique_ptr_static_vector<ExprAST>(std::move(else_if_expr)));
    }
    Log::Info() << "Tok before verif tok_else : " << CurTok << "\n";
    if (CurTok == tok_else){
      goto else_start;
    }
  } else {
  Else = ParseExpression();
  }
  }
  Log::Info() << "CurTok at end of IfExprAST : " << CurTok << "\n";
  return std::make_unique<IfExprAST>(IfLoc, std::move(Cond), std::move(Then),
                                      std::move(Else));
}


std::unique_ptr<ExprAST> ParseReturn(){
  getNextToken(); // eat the return
  Log::Info() << "CurTok return : " << CurTok << " " << NumVal << "\n";
  if (CurTok == tok_identifier && IdentifierStr == "void"){
    getNextToken();
    return std::make_unique<ReturnAST>(nullptr);
  }
  auto return_value = ParseExpression();
  if (!return_value){
    return nullptr;
  }
  Log::Info() << "CurTok return after : " << CurTok << "\n";
  return std::make_unique<ReturnAST>(std::move(return_value));
}

std::unique_ptr<ExprAST> ParseBreakExpr(){
  getNextToken();
  return std::make_unique<BreakExprAST>();
}

std::unique_ptr<ExprAST> ParseGotoExpr(){
 getNextToken();
 std::string label_name = IdentifierStr;
 Log::Info() << "label_name in goto : " << label_name << "\n";
 getNextToken();
 return std::make_unique<GotoExprAST>(label_name);
}

std::unique_ptr<ExprAST> ParseLabelExpr(){
  getNextToken(); // eat the label
  std::string label_name = IdentifierStr.substr(0, IdentifierStr.size());
  Log::Info() << "label_name in label : " << label_name << "\n";
  getNextToken();
  return std::make_unique<LabelExprAST>(label_name);
}

std::unique_ptr<ExprAST> ParseMatch(){
    getNextToken();
    std::vector<std::unique_ptr<matchCase>> matchCases;
    std::string matchVar = IdentifierStr;
    getNextToken();
    if (CurTok != '{'){
        return LogError("Missing '{' in match");
    }
    getNextToken();
    Log::Info() << "CurTok parsing match case : " << CurTok << "\n";
    while (CurTok != '}'){
        std::string VarName = "";
        std::unique_ptr<ExprAST> Body;
        std::unique_ptr<ExprAST> expr = nullptr;
        std::string enum_name = "";
        std::string enum_member_name = "";
        bool is_underscore = false;
        Log::Info() << "match case IdentifierStr : " << IdentifierStr << "\n";
        if (IdentifierStr == "_"){
            Log::Info() << "Is underscore match case" << "\n";
            is_underscore = true;
            getNextToken();
        } else {
        if (CurTok == tok_identifier){
        enum_name = IdentifierStr;
        Log::Info() << "Match case enum_name : " << enum_name << "\n";
        getNextToken();
        if (CurTok != ':'){
            return LogError("Missing \"::\" in match enum member case");
        }
        getNextToken();
        if (CurTok != ':'){
            return LogError("Missing \"::\" in match enum member case");
        }
        getNextToken();
        enum_member_name = IdentifierStr;
        Log::Info() << "Match case enum_member_name : " << enum_member_name << "\n";
        getNextToken();
        if (CurTok == '('){
            getNextToken();
            VarName = IdentifierStr;
            Log::Info() << "Match case found VarName : " << VarName << "\n";
            getNextToken();
            if (CurTok != ')'){
                return LogError("Missing ) in value of match enum case");
            }
            getNextToken();
        }
        } else {
            expr = ParseExpression();
        }
        }
        Log::Info() << "CurTok match : " << CurTok << "\n";
        Log::Info() << "OpStringMultiChar : " << OpStringMultiChar << "\n";
        if (CurTok != tok_op_multi_char && OpStringMultiChar != "=>"){
            return LogError("Missing \"=>\" before match enum case body");
        }
        getNextToken();
        Log::Info() << "CurTok match 2 : " << CurTok << "\n";
        if (CurTok != '>'){
            return LogError("Missing \"=>\" before match enum case body");
        }
        getNextToken();


        auto Expr = ParseExpression();
        bool is_body_expr = false;
        if (dynamic_cast<ScopeExprAST*>(Expr.get())){
            is_body_expr = true;
        }
        Body = std::move(Expr);
        if (!is_body_expr){
        if (CurTok != ','){
            return LogError("Expected ',' after single line body of match enum case");
        }
        getNextToken();
        }
        matchCases.push_back(std::make_unique<matchCase>(std::move(expr), enum_name, enum_member_name, VarName, is_underscore, std::move(Body)));
    }
    getNextToken();
    return std::make_unique<MatchExprAST>(matchVar, std::move(matchCases));
}

std::unique_ptr<ExprAST> ParseDefer(){
    getNextToken();
    std::unique_ptr<ExprAST> Expr = ParseExpression(); 
    return std::make_unique<DeferExprAST>(std::move(Expr));
}

std::unique_ptr<ExprAST> ParseClosure(){
    std::unique_ptr<ExprAST> ret;
    std::vector<std::unique_ptr<ExprAST>> Body; 
    getNextToken();
    if (CurTok != '('){
        return LogError("Missing '(' in closure");
    }
    getNextToken();
    std::vector<std::pair<std::string, Cpoint_Type>> ArgNames;
    bool is_variable_number_args = false;
    ret = ParseFunctionArgsTyped(ArgNames, is_variable_number_args);
    if (!ret){
        return nullptr;
    }
    getNextToken();
    std::vector<std::string> captured_vars;
    if (CurTok == '|'){
        getNextToken();
        while (1){
            if (!var_exists(IdentifierStr)){
                return LogError("Variable %s captured by closure doesn't exist", IdentifierStr.c_str());
            }
            captured_vars.push_back(IdentifierStr);
            getNextToken();
            if (CurTok == '|'){
                break;
            }
            Log::Info() << "closure Curtok : " << CurTok << "\n";
            if (CurTok != ','){
                return LogError("Expected '|' or ',' in captured vars list in closure");
            }
            getNextToken();
        }
        getNextToken();
    }
    
    Cpoint_Type return_type = Cpoint_Type(double_type);
    if (CurTok == tok_identifier || CurTok == tok_struct || CurTok == tok_class){
        return_type = ParseTypeDeclaration(false);
    }
    if (CurTok != '{'){
        return LogError("Missing '{' in closure");
    }
    getNextToken();
    ret = ParseBodyExpressions(Body, false);
    if (!ret){
        return nullptr;
    }
    getNextToken();
    return std::make_unique<ClosureAST>(std::move(Body), ArgNames, return_type, captured_vars);
}

std::unique_ptr<ExprAST> ParseLoopExpr(){
  getNextToken();  // eat the loop.
  std::vector<std::unique_ptr<ExprAST>> Body;
  if (CurTok == '{'){
    // infinite loop like in rust
    getNextToken();
    auto ret = ParseBodyExpressions(Body);
    if (!ret){
        return nullptr;
    }
    getNextToken();
    return std::make_unique<LoopExprAST>("", nullptr, std::move(Body), true);
  } else {
    Log::Info() << "Loop In Expr" << "\n";
    if (CurTok != tok_identifier){
      return LogErrorL("not an identifier or '{' after the loop");
    }
    std::string VarName = IdentifierStr;
    getNextToken();
    if (CurTok != tok_in){
      return LogErrorL("missing 'in' in loop");
    }
    getNextToken();
    //auto Array = ParseExpression();
    auto Array = ParseIdentifierExpr();
    if (CurTok != '{'){
      return LogErrorL("missing '{' in loop in");
    }
    getNextToken();
    auto ret = ParseBodyExpressions(Body);
    if (!ret){
        return nullptr;
    }
    getNextToken();

    return std::make_unique<LoopExprAST>(VarName, std::move(Array), std::move(Body));
  }
}

std::unique_ptr<ExprAST> ParseWhileExpr(){ 
  getNextToken();  // eat the while.
  Log::Info() << "Before Parse Expression" << "\n";
  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;
  Log::Info() << "first CurTok : " << CurTok << "\n";
  std::unique_ptr<ExprAST> Body = ParseExpression();
  if (!Body){
    return nullptr;
  }
  Log::Info() << "CurTok : " << CurTok << "\n";
  return std::make_unique<WhileExprAST>(std::move(Cond), std::move(Body));
}

std::unique_ptr<ExprAST> ParseForExpr() {
  getNextToken();  // eat the for.

  if (CurTok != tok_identifier)
    return LogError("expected identifier after for");

  std::string IdName = IdentifierStr;
  getNextToken();  // eat identifier.
  Cpoint_Type varType = Cpoint_Type(double_type);
  if (CurTok == ':'){
    varType = ParseTypeDeclaration();
  }
  if (CurTok != '=')
    return LogError("expected '=' after for");
  getNextToken();  // eat '='.


  auto Start = ParseExpression();
  if (!Start)
    return nullptr;
  if (CurTok != ',')
    return LogError("expected ',' after for start value");
  getNextToken();
  Log::Info() << "Before parse expression for" << "\n";
  auto End = ParseExpression();
  if (!End)
    return nullptr;

  // The step value is optional.
  std::unique_ptr<ExprAST> Step;
  if (CurTok == ',') {
    getNextToken();
    Step = ParseExpression();
    if (!Step)
      return nullptr;
  }
  std::unique_ptr<ExprAST> Body = ParseExpression();
  Log::Info() << "CurTok : " << CurTok << "\n";
  return std::make_unique<ForExprAST>(IdName, varType, std::move(Start),
                                       std::move(End), std::move(Step),
                                       std::move(Body));
}

std::unique_ptr<ExprAST> ParseVarExpr() {
  getNextToken(); // eat the var.

  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
  Cpoint_Type cpoint_type = Cpoint_Type(double_type);
  bool is_array = false;
  std::unique_ptr<ExprAST> index = nullptr;
  bool infer_type = false;
  // At least one variable name is required.
  if (CurTok != tok_identifier)
    return LogError("expected identifier after var");
  // TODO : maybe verify the multiple var code and make it register all vars
  while (true) {
    std::string Name = IdentifierStr;
    getNextToken(); // eat identifier.
    // if declaration of array
    if (CurTok == '['){
      is_array = true;
      getNextToken();
      index = ParseNumberExpr();
      if (CurTok != ']'){
        return LogError("missing ']'");
      }
      getNextToken();
    }
    infer_type = false;
    if (CurTok == ':'){
      Log::Info() << "Parse Type Declaration Var" << "\n";
      cpoint_type = ParseTypeDeclaration();
    } else {
      Log::Info() << "Infering type (CurTok : " << CurTok << ")" << "\n";
      infer_type = true;
    }
    if (is_array){
        cpoint_type.is_array = is_array;
        cpoint_type.nb_element = from_val_to_int(index/*->clone()*/->codegen());;
    }
    // Read the optional initializer.
    std::unique_ptr<ExprAST> Init = nullptr;
    if (CurTok == '=') {
      getNextToken(); // eat the '='.

      Init = ParseExpression();
      if (!Init)
        return nullptr;
    }
    if (infer_type){
        if (Init){
            cpoint_type = Init->get_type();
        } else {
            Log::Info() << "Missing type declaration and default value to do type inference. Type defaults to double" << "\n";
            cpoint_type.type = double_type;
        }
    }
    VarNames.push_back(std::make_pair(Name, std::move(Init)));

    // End of var list, exit loop.
    if (CurTok != ',')
      break;
    getNextToken(); // eat the ','.

    if (CurTok != tok_identifier)
      return LogError("expected identifier list after var");
  }

  cpoint_type.is_struct = cpoint_type.struct_name != "";
  NamedValues[VarNames.at(0).first] = std::make_unique<NamedValue>(nullptr, cpoint_type);
  return std::make_unique<VarExprAST>(std::move(VarNames), cpoint_type, std::move(index), infer_type);
}

extern std::ofstream file_log;
extern bool last_line;
extern bool is_in_extern;

static void HandleStruct(std::vector<std::unique_ptr<StructDeclarAST>>& structs){
  if (auto structAST = ParseStruct()){
    structs.push_back(std::move(structAST));
  } else {
    if (!is_template_parsing_struct){
      getNextToken();
    }
  }
}

#if ENABLE_FILE_AST

extern void HandleTest();

std::unique_ptr<FileAST> ParseFile(){
  std::vector<std::unique_ptr<GlobalVariableAST>> global_vars;
  std::vector<std::unique_ptr<StructDeclarAST>> structs;
  std::vector<std::unique_ptr<UnionDeclarAST>> unions;
  std::vector<std::unique_ptr<EnumDeclarAST>> enums;
  std::vector<std::unique_ptr<MembersDeclarAST>> members;
  std::vector<std::unique_ptr<PrototypeAST>> function_protos;
  std::vector<std::unique_ptr<FunctionAST>> functions;
  std::vector<std::unique_ptr<ModAST>> mods;
  while (1) {
    if (Comp_context->debug_mode){
    std::flush(file_log);
    }
    if (last_line == true){
      if (Comp_context->debug_mode){
      file_log << "exit" << "\n";
      }
      goto after_loop;
      // return;
    }
    switch (CurTok) {
    case tok_eof:
      last_line = true; 
      goto after_loop;
      // return;
    case '#':
        // found call macro
        break;
    case ';': // ignore top-level semicolons.
      getNextToken();
      break;
    case tok_func:
      if (auto FnAST = ParseDefinition()){
        functions.push_back(std::move(FnAST));
      } else {
        if (!is_template_parsing_definition){
          // Skip token for error recovery.
          getNextToken();
        }
      }
      break;
    case tok_extern:
      is_in_extern = true;
      if (auto ProtoAST = ParseExtern()){
        function_protos.push_back(std::move(ProtoAST));
      } else {
        getNextToken();
      }
      is_in_extern = false;
      break;
    case tok_struct:
      HandleStruct(structs);
      break;
    case tok_union:
      if (auto unionAST = ParseUnion()){
        unions.push_back(std::move(unionAST));
      } else {
        getNextToken();
      }
      break;
    case tok_enum:
      if (auto enumAST = ParseEnum()){
        enums.push_back(std::move(enumAST));
      } else {
        if (!is_template_parsing_enum){
            getNextToken();
        }
      }
      break;
    case tok_class:
      HandleStruct(structs);
      break;
    case tok_var:
      if (auto GlobalVarAST = ParseGlobalVariable()){
        global_vars.push_back(std::move(GlobalVarAST));
      } else {
        getNextToken();
      }
      break;
    case tok_typedef:
      if (auto typeDefAST = ParseTypeDef()){
        // no need to codegen it because it is a noop
      } else {
        getNextToken();
      }
      break;
    case tok_mod: {
      auto modAST = ParseMod();
      mods.push_back(std::move(modAST));
      break;
    }
    case tok_members:
      if (auto memberAST = ParseMembers()){
        members.push_back(std::move(memberAST));
      } else {
        getNextToken();
      }
      break;
    case tok_single_line_comment:
      Log::Info() << "found single-line comment" << "\n";
      Log::Info() << "char found as a '/' : " << CurTok << "\n";
      if (Comp_context->debug_mode){
      file_log << "found single-line comment" << "\n";
      }
      HandleComment();
      break;
    default:
      //bool is_in_module_context = CurTok == '}' && !modulesNamesContext.empty();
      /*if (is_in_module_context){
        modulesNamesContext.pop_back();
        getNextToken();
      } else {*/
      if (CurTok == tok_identifier && IdentifierStr == "test"){
        HandleTest();
        break;
      }
#if ENABLE_JIT
    if (Comp_context->jit_mode){
      if (CurTok != '\0'){
        HandleTopLevelExpression();
      } else {
        getNextToken();
      }
      break;
    }
#endif
      Log::Info() << "CurTok : " << CurTok << "\n";
      Log::Info() << "identifier : " << IdentifierStr << "\n";
      LogError("TOP LEVEL EXPRESSION FORBIDDEN");
      getNextToken();
      //}
      break;
    }
  }
after_loop:
  return std::make_unique<FileAST>(std::move(global_vars), std::move(structs), std::move(unions), std::move(enums), std::move(members), std::move(function_protos), std::move(functions), std::move(mods));
}

#endif