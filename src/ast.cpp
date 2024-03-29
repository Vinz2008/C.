//#include <map>
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
#include "builtin_macros.h"

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
//extern bool std_mode;
//extern bool gc_mode;
extern std::unique_ptr<Module> TheModule;
extern std::vector<std::string> types;
extern std::vector</*std::string*/ Cpoint_Type> typeDefTable;

extern std::unordered_map<std::string, std::unique_ptr<TemplateProto>> TemplateProtos;
extern std::unordered_map<std::string, std::unique_ptr<StructDeclar>> TemplateStructDeclars;
extern std::vector<std::string> modulesNamesContext;
extern std::pair<std::string, /*std::string*/ Cpoint_Type> TypeTemplateCallCodegen;
extern std::vector<std::unique_ptr<TemplateStructCreation>> StructTemplatesToGenerate;
extern std::string TypeTemplateCallAst;
extern std::unordered_map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;

extern std::unique_ptr<Preprocessor::Context> context;

Cpoint_Type ParseTypeDeclaration(bool eat_token);

bool is_comment = false;

extern void HandleComment();
extern std::string filename;

bool is_template_parsing_definition = false;
bool is_template_parsing_struct = false;

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


std::unique_ptr<ExprAST> StructMemberCallExprAST::clone(){
    return std::make_unique<StructMemberCallExprAST>(get_Expr_from_ExprAST<BinaryExprAST>(StructMember->clone()), clone_vector<ExprAST>(Args));
}

std::unique_ptr<ExprAST> NEWCallExprAST::clone(){
    return std::make_unique<NEWCallExprAST>(function_expr->clone(), clone_vector<ExprAST>(Args), template_passed_type);
}

std::unique_ptr<ExprAST> ConstantArrayExprAST::clone(){
    return std::make_unique<ConstantArrayExprAST>(clone_vector<ExprAST>(ArrayMembers));
}

std::unique_ptr<ExprAST> ConstantStructExprAST::clone(){
    return std::make_unique<ConstantStructExprAST>(struct_name, clone_vector<ExprAST>(StructMembers));
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
        //std::string name = VarNames.at(i).first;
        /*std::unique_ptr<ExprAST> val;
        if (VarNames.at(i).second != nullptr){
        val = VarNames.at(i).second->clone();
        } else {
          val = nullptr;
        }*/
        //VarNamesCloned.push_back(std::make_pair(name, std::move(val)));
        //VarNamesCloned.push_back(std::make_pair(VarNames.at(i).first, std::move(val)));
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

std::unique_ptr<ExprAST> RedeclarationExprAST::clone(){
    std::unique_ptr<ExprAST> indexCloned = nullptr;
    if (index != nullptr){
      indexCloned = index->clone();
    }
    return std::make_unique<RedeclarationExprAST>(VariableName, Val->clone(), member, std::move(indexCloned));
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
        if (!VarTemp){
            return LogErrorS("Vars in struct is not a var and is an other type exprAST");
        }
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
    return std::make_unique<EnumDeclarAST>(Name, enum_member_contain_type, clone_vector<EnumMember>(EnumMembers));
}

std::unique_ptr<TestAST> TestAST::clone(){
    return std::make_unique<TestAST>(description, clone_vector<ExprAST>(Body));
}

std::unique_ptr<ExprAST> IfExprAST::clone(){
    return std::make_unique<IfExprAST>(this->loc, Cond->clone(), clone_vector<ExprAST>(Then), clone_vector<ExprAST>(Else));
}

std::unique_ptr<ExprAST> ForExprAST::clone(){
    return std::make_unique<ForExprAST>(VarName, VarType, Start->clone(), End->clone(), Step->clone(), clone_vector<ExprAST>(Body));
}

std::unique_ptr<ExprAST> WhileExprAST::clone(){
    return std::make_unique<WhileExprAST>(Cond->clone(), clone_vector<ExprAST>(Body));
}

std::unique_ptr<ExprAST> LoopExprAST::clone(){
    return std::make_unique<LoopExprAST>(VarName, Array->clone(), clone_vector<ExprAST>(Body), is_infinite_loop);
}

std::unique_ptr<matchCase> matchCase::clone(){
    std::unique_ptr<ExprAST> exprCloned;
    if (expr){
        exprCloned = expr->clone();
    }
    return std::make_unique<matchCase>(std::move(exprCloned), enum_name, enum_member, var_name, is_underscore, clone_vector<ExprAST>(Body));
}

std::unique_ptr<ExprAST> MatchExprAST::clone(){
    return std::make_unique<MatchExprAST>(matchVar, clone_vector<matchCase>(matchCases));
}

std::unique_ptr<ExprAST> ClosureAST::clone(){
    return std::make_unique<ClosureAST>(clone_vector<ExprAST>(Body), ArgNames, return_type,captured_vars);
}

std::unique_ptr<ArgsInlineAsm> ArgsInlineAsm::clone(){
    return std::make_unique<ArgsInlineAsm>(/*assembly_code->clone()*/ std::unique_ptr<StringExprAST>(dynamic_cast<StringExprAST*>(assembly_code->clone().release())), clone_vector<ArgInlineAsm>(InputOutputArgs));
}

void generate_gc_init(std::vector<std::unique_ptr<ExprAST>>& Body){
    std::vector<std::unique_ptr<ExprAST>> Args_gc_init;
    auto E_gc_init = std::make_unique<CallExprAST>(emptyLoc, "gc_init", std::move(Args_gc_init), Cpoint_Type());
    Body.push_back(std::move(E_gc_init));
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
    /*if (CurTok != ','){
      return LogError("missing \',\' in constant array");
    }
    getNextToken();*/
    member_nb++;
  }
  return std::make_unique<ConstantArrayExprAST>(std::move(ArrayMembers));
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;
  struct Source_location IdLoc = Comp_context->curloc;

  Log::Info() << "Parse Identifier Str" << "\n";
  getNextToken();  // eat identifier.
  std::string member = "";
  std::unique_ptr<ExprAST> indexAST = nullptr;
  //bool is_array = false;
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
      module_mangled_function_name = module_function_mangling(module_mangled_function_name, second_part);
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
    /*if (CurTok != '{'){
        return LogError("Missing '{' in struct constant declaration");
    }*/
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

  //if (CurTok != '(' && CurTok != /*'<'*/ '~'){ // Simple variable ref.
    Log::Info() << "VariableExprAST" << "\n";
    std::unique_ptr<Cpoint_Type> type;
    if (NamedValues[IdName] == nullptr && GlobalVariables[IdName] == nullptr){
      type = std::make_unique<Cpoint_Type>(double_type);
    } else if (GlobalVariables[IdName] != nullptr){
      type = std::make_unique<Cpoint_Type>(GlobalVariables[IdName]->type);
    } else  {
      type = std::make_unique<Cpoint_Type>(NamedValues[IdName]->type);
    }
    return std::make_unique<VariableExprAST>(IdLoc, IdName, *type);
  //}
  // Call.
  Cpoint_Type template_passed_type = Cpoint_Type();
  if (CurTok == '~'){
    getNextToken();
    template_passed_type = /*IdentifierStr*/ ParseTypeDeclaration(false);
    //getNextToken();
    if (CurTok != '~'){
      return LogError("expected '~' not found");
    }
    getNextToken();
    }
    fprintf(stderr, "should use the binop implementation of callexpr\n");
    exit(1);
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
    return std::make_unique<NEWCallExprAST>(std::move(LHS), std::move(Args), template_passed_type);
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  Log::Info() << "PARSING BINOP" << "\n";
  Log::Info() << "ExprPrec : " << ExprPrec << "\n";
  // If this is a binop, find its precedence.
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
    //if (CurTok != '=' && CurTok != '|' && CurTok != '!'){
    //Log::Info() << "CurTok before if : " << CurTok << "\n";
    //if (CurTok != tok_op_multi_char){
    if (TokPrec < ExprPrec){
      return LHS;
    }
    //}
    //}

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
    if (BinOp == "(" || BinOp == "~"){
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
        Log::Info() << "CurTok ARRAY_MEMBER_OPERATOR_IMPL : " << CurTok << "\n";
        Log::Info() << LHS->to_string() << "[" << RHS->to_string() << "]" << "\n";
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
  case tok_func:
    return ParseClosure();
  }
}

std::unique_ptr<ExprAST> ParseMacroCall(){
    std::vector<std::unique_ptr<ExprAST>> ArgsMacro;
    std::unique_ptr<ArgsInlineAsm> argsInlineAsm;
    Log::Info() << "Parsing macro call" << "\n";
    getNextToken();
    std::string function_name = IdentifierStr;
    getNextToken();
    if (CurTok != '('){
        return LogError("missing '(' in the call of a macro function");
    }
    getNextToken();
    if (function_name == "asm"){
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
        /*if (CurTok != ')'){
            return LogError("Missing ')' in asm macro");
        }*/
        getNextToken(); // eat ')'
        argsInlineAsm = std::make_unique<ArgsInlineAsm>(std::move(asm_code), std::move(InputOutputArgs));
        return generate_asm_macro(std::move(argsInlineAsm));
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
        return stringify_macro(std::move(ArgsMacro.at(0)));
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
        return generate_print_macro(ArgsMacro, false);
    } else if (function_name == "println"){
        return generate_print_macro(ArgsMacro, true);
    } else if (function_name == "unreachable"){
        return generate_unreachable_macro();
    } else if (function_name == "assume"){
        return generate_assume_macro(ArgsMacro);
    }
    }
    return LogError("unknown function macro called : %s", function_name.c_str());
}

std::unique_ptr<ExprAST> ParseSemiColon(){
    return std::make_unique<SemicolonExprAST>();
}

Cpoint_Type ParseTypeDeclaration(bool eat_token = true){
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
  bool is_struct_template = false;
  Cpoint_Type* struct_template_type_passed = nullptr;
  //Log::Info() << "type declaration found" << "\n";
  if (eat_token){
  getNextToken(); // eat the ':'
  }
  if (CurTok != tok_identifier && CurTok != tok_struct && CurTok != tok_class && CurTok != tok_func && CurTok != tok_union && CurTok != tok_enum){
    LogError("expected identifier after var in type declaration");
    return default_type;
  }
  /*if (CurTok == tok_identifier && IdentifierStr == TypeNameTemplateAST){

  }*/ // TODO make a way to recognize a template type in cpoint_type. Remove the following code because the replacement of the type will be made in codegen
  if (CurTok == tok_identifier && IdentifierStr == TypeTemplateCallCodegen.first){
    Log::Info() << "Found template type in template call" << "\n";
    //IdentifierStr = TypeTemplateCallCodegen.second;
    Cpoint_Type temp_type =  TypeTemplateCallCodegen.second;
    /*getNextToken();
    if (CurTok == tok_ptr){
        temp_type.is_ptr = true;
        temp_type.nb_ptr++;
        getNextToken();
    }*/
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
  if (CurTok == tok_struct || CurTok == tok_class){
    getNextToken();
    struct_Name = IdentifierStr;
    Log::Info() << "struct_Name " << struct_Name << "\n";
    getNextToken();
    if (CurTok == '~'){
        Log::Info() << "found templates for struct" << "\n";
        getNextToken();
        struct_template_type_passed = new Cpoint_Type(ParseTypeDeclaration(false));
        if (!struct_template_type_passed){
            LogError("missing template type for struct");
            return default_type;
        }
        if (CurTok != '~'){
            LogError("Missing '~' in struct template type usage");
            return default_type;
        }
        getNextToken();
        auto structDeclar = TemplateStructDeclars[struct_Name]->declarAST->clone();
        structDeclar->Name = get_struct_template_name(struct_Name, /*struct_template_name*/ *struct_template_type_passed);
        StructTemplatesToGenerate.push_back(std::make_unique<TemplateStructCreation>(/*struct_template_name*/ *struct_template_type_passed, std::move(structDeclar)));
        Log::Info() << "added to StructTemplatesToGenerate" << "\n";
        is_struct_template = true;
    }
    while (CurTok == tok_ptr){
      is_ptr = true;
      nb_ptr++;
      Log::Info() << "Type Declaration ptr added : " << nb_ptr << "\n";
      getNextToken();
    }
  } else if (CurTok == tok_union){
    getNextToken();
    unionName = IdentifierStr;
    getNextToken();
    if (CurTok == tok_ptr){
      is_ptr = true;
      getNextToken();
    }
  } else if (CurTok == tok_enum){
    getNextToken();
    enumName = IdentifierStr;
    getNextToken();
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
    LogError("wrong type %s found", IdentifierStr.c_str());
    return default_type;
  }
before_gen_cpoint_type:
  return Cpoint_Type(type, is_ptr, nb_ptr, false, 0, struct_Name != "", struct_Name, unionName != "", unionName, enumName != "", enumName, is_template_type, is_struct_template, /*struct_template_name*/ struct_template_type_passed, is_function, args, return_type);
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

std::unique_ptr<ExprAST> ParseBodyExpressions(std::vector<std::unique_ptr<ExprAST>>& Body, bool is_func_body){
    bool return_found = false;
    while (CurTok != '}'){
      auto E = ParseExpression();
      if (!E)
        return nullptr;
      if (is_func_body){
      if (dynamic_cast<ReturnAST*>(E.get())){
        return_found = true;
      }
      }
      if (!return_found){
        Body.push_back(std::move(E));
      }
    }
    return std::make_unique<EmptyExprAST>(); 
}

std::unique_ptr<ExprAST> ParseBool(bool bool_value){
  Log::Info() << "Parsing bool" << "\n";
  getNextToken();
  return std::make_unique<BoolExprAST>(bool_value);
}

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
  if (CurTok == /*'<'*/ '~'){
    getNextToken();
    template_name = IdentifierStr;
    getNextToken();
    if (CurTok != /*'>'*/ '~'){
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
    ArgNames.push_back(std::make_pair("argc", Cpoint_Type(int_type, false)));
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
  if (CurTok == tok_identifier || CurTok == tok_struct || CurTok == tok_class){
    Log::Info() << "Tok type : " << CurTok << "\n";
    Log::Info() << "type : " << IdentifierStr << "\n";
    cpoint_type = ParseTypeDeclaration(false);
  }
  if (!modulesNamesContext.empty()){
    std::string module_mangled_function_name = FnName;
    for (int i = modulesNamesContext.size()-1; i >= 0; i--){
      module_mangled_function_name = module_function_mangling(modulesNamesContext.at(i), module_mangled_function_name);
    }
    FnName = module_mangled_function_name;
    Log::Info() << "mangled name is : " << FnName << "\n";
  }
  Log::Info() << "FnLoc in ast : " << FnLoc << "\n";
  return std::make_unique<PrototypeAST>(FnLoc, FnName, ArgNames, cpoint_type, Kind != 0, BinaryPrecedence, is_variable_number_args, has_template, template_name);
}

std::unique_ptr<StructDeclarAST> ParseStruct(){
  int i = 0;
  std::string template_name = "";
  bool has_template = false;
  std::vector<std::unique_ptr<VarExprAST>> VarList;
  std::vector<std::unique_ptr<FunctionAST>> Functions;
  std::vector<std::unique_ptr<PrototypeAST>> ExternFunctions;
  getNextToken(); // eat struct
  std::string structName = IdentifierStr;
  Log::Info() << "Struct Name : " << structName << "\n";
  getNextToken();
  if (CurTok == '~'){
    getNextToken();
    template_name = IdentifierStr;
    getNextToken();
    if (CurTok != '~'){
      return LogErrorS("Missing '~' in template struct declaration");
    }
    getNextToken();
    has_template = true;
    TypeTemplateCallAst = template_name;
  }
  if (CurTok != '{'){
    return LogErrorS("Expected '{' in Struct");
  }
  getNextToken();
  while ((CurTok == tok_var || CurTok == tok_func || CurTok == tok_extern) && CurTok != '}'){
    Log::Info() << "Curtok in struct parsing : " << CurTok << "\n";
    if (CurTok == tok_var){
        auto exprAST = ParseVarExpr();
        if (!exprAST){
            return nullptr;
        }
        std::unique_ptr<VarExprAST> declar = get_Expr_from_ExprAST<VarExprAST>(std::move(exprAST));
        if (!declar){
            return LogErrorS("Error in struct declaration vars");
        }
        VarList.push_back(std::move(declar));
    } else if (CurTok == tok_extern){
        getNextToken();
        auto protoExpr = ParsePrototype();
        PrototypeAST* protoPtr = dynamic_cast<PrototypeAST*>(protoExpr.get());
        if (protoPtr == nullptr){
            return LogErrorS("Error in struct declaration externs");
        }
        std::unique_ptr<PrototypeAST> Proto;
        protoExpr.release();
        Proto.reset(protoPtr);
        if (Proto == nullptr){ return nullptr; }
        ExternFunctions.push_back(std::move(Proto));
        Log::Info() << "CurTok after extern : " << CurTok << "\n";
        
    } else if (CurTok == tok_func){
      Log::Info() << "function found in struct" << "\n";
      auto funcAST = ParseDefinition();
      Log::Info() << "AFTER ParseDefinition" << "\n";
      FunctionAST* functionAST = dynamic_cast<FunctionAST*>(funcAST.get());
      if (functionAST == nullptr){
      return LogErrorS("Error in struct declaration funcs");
      }
      std::unique_ptr<FunctionAST> declar;
      funcAST.release();
      declar.reset(functionAST);
      if (declar == nullptr){return nullptr;}
      Functions.push_back(std::move(declar));
      Log::Info() << "Function Number in struct : " << i+1 << "\n";
      i++;
    }
  }
  Log::Info() << "CurTok : " << CurTok << "\n";
  if (CurTok != '}'){
    return LogErrorS("Expected '}' in struct");
  }
  getNextToken();  // eat '}'.
  auto structDeclar = std::make_unique<StructDeclarAST>(structName, std::move(VarList), std::move(Functions), std::move(ExternFunctions), has_template, template_name);
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
            return LogErrorMembers("Unkown expression in members block");
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
    return std::make_unique<EnumDeclarAST>(Name, enum_member_contain_type, std::move(EnumMembers));
}

std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken();  // eat func.
  auto Proto = ParsePrototype();
  if (!Proto) return nullptr;
  auto argsCopy(Proto->Args);
  auto ProtoCopy = Proto->clone();
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
  Cpoint_Type type = Cpoint_Type(double_type);
  bool is_variable = false;
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
  }
  auto Sizeof = std::make_unique<SizeofExprAST>(type, is_variable, Name);
  Log::Info() << "after sizeof : " << Name << "\n";
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

std::unique_ptr<ExprAST> ParseUnary() {
  Log::Info() << "PARSE UNARY CurTok : " << CurTok << "\n";
  // If the current token is not an operator, it must be a primary expr.
  if (!isascii(CurTok) || CurTok == '(' || CurTok == ',' || CurTok == '{' || CurTok == ':' || CurTok == tok_string || CurTok == tok_false || CurTok == tok_true || CurTok == '[' || CurTok == '#' || CurTok == ';')
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
  //std::string value_type = IdentifierStr;
  //getNextToken();
  types.push_back(new_type);
  typeDefTable.push_back(value_type);
  return std::make_unique<TypeDefAST>(new_type, value_type);
}

std::unique_ptr<ModAST> ParseMod(){
  getNextToken();
  std::string mod_name = IdentifierStr;
  getNextToken();
  if (CurTok != '{'){
    return LogErrorM("missing '{' in mod");
  }
  getNextToken();
  modulesNamesContext.push_back(mod_name);
  return std::make_unique<ModAST>(mod_name);
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
    auto section_name_expression = static_cast<StringExprAST*>(ParseStrExpr().get());
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
  if (CurTok != '{'){
    Log::Info() << "CurTok : " << CurTok << "\n";
    return LogError("expected {");
  }
  getNextToken();  // eat the {
  std::vector<std::unique_ptr<ExprAST>> Then;
  auto ret = ParseBodyExpressions(Then);
  if (!ret){
    return nullptr;
  }
  Log::Info() << "CurTok : " << CurTok << "\n";
  if (CurTok != '}'){
    return LogError("expected } in if");
  }
  getNextToken();
  std::vector<std::unique_ptr<ExprAST>> Else;
else_start:
  if (CurTok == tok_else){
  getNextToken();
  if (CurTok == tok_if){
    auto else_if_expr = ParseIfExpr();
    if (!else_if_expr){
      return nullptr;
    }
    Else.push_back(std::move(else_if_expr));
    //getNextToken();
    if (CurTok == tok_else){
      goto else_start;
    }
  } else {
  if (CurTok != '{'){
    return LogError("expected {");
  }
  getNextToken();
  auto ret = ParseBodyExpressions(Else);
  if (!ret){
    return nullptr;
  }
  if (CurTok != '}'){
    return LogError("expected } in else");
  }
  getNextToken();
  }
  }
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
  //auto Result = std::make_unique<ReturnAST>(NumVal);
  if (!return_value){
    return nullptr;
  }
  //getNextToken();
  Log::Info() << "CurTok return after : " << CurTok << "\n";
  return std::make_unique<ReturnAST>(std::move(return_value));
  //return std::move(Result);
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
        std::vector<std::unique_ptr<ExprAST>> Body;
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
        if (CurTok == '{'){
            getNextToken();
            auto ret = ParseBodyExpressions(Body);
            if (!ret){
                return nullptr;
            }
            getNextToken(); // eat }
        } else {
            auto Expr = ParseExpression();
            Body.push_back(std::move(Expr));
            if (CurTok != ','){
                return LogError("Expected ',' after single line body of match enum case");
            }
            getNextToken();
        }
        matchCases.push_back(std::make_unique<matchCase>(std::move(expr), enum_name, enum_member_name, VarName, is_underscore, std::move(Body)));
    }
    /*if (CurTok != '}'){
        return LogError("Missing '}' in match");
    }*/
    getNextToken();
    return std::make_unique<MatchExprAST>(matchVar, std::move(matchCases));
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
    /*if (CurTok != ')'){
        return LogError("Missing '(' in closure");
    }*/
    getNextToken();
    std::vector<std::string> captured_vars;
    if (CurTok == '|'){
        getNextToken();
        while (1){
            captured_vars.push_back(IdentifierStr);
            getNextToken();
            if (CurTok == '|'){
                break;
            }
            if (CurTok != ','){
                return LogError("Expected '|' or ',' in captured vars list in closure");
            }
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
  if (CurTok != '{'){
    return LogError("expected {");
  }
  getNextToken();
  std::vector<std::unique_ptr<ExprAST>> Body;
  auto ret = ParseBodyExpressions(Body);
  if (!ret){
    return nullptr;
  }
  Log::Info() << "CurTok : " << CurTok << "\n";
  if (CurTok != '}'){
    return LogError("expected }");
  }
  getNextToken();
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

  /*if (CurTok != tok_in)
    return LogError("expected 'in' after for");*/
  if (CurTok != '{'){
    Log::Info() << "token befor : " << CurTok << "\n";
    return LogError("expected { after for");
  }
  std::vector<std::unique_ptr<ExprAST>> Body;
  getNextToken();  // eat '{'.
  auto ret = ParseBodyExpressions(Body);
  if (CurTok != '}'){
    return LogError("expected } after for");
  }
  getNextToken();

  return std::make_unique<ForExprAST>(IdName, varType, std::move(Start),
                                       std::move(End), std::move(Step),
                                       std::move(Body));
}

std::unique_ptr<ExprAST> ParseVarExpr() {
  getNextToken(); // eat the var.

  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
  Cpoint_Type cpoint_type = Cpoint_Type(double_type);
  /*int type = double_type;
  bool is_ptr = false;
  //int nb_element = 0;*/
  bool is_array = false;
  /*std::string struct_name = "";
  //std::string class_name = "";
  int nb_ptr = 0;*/
  std::unique_ptr<ExprAST> index = nullptr;
  bool infer_type = false;
  // At least one variable name is required.
  if (CurTok != tok_identifier)
    return LogError("expected identifier after var");
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
      cpoint_type = ParseTypeDeclaration(/*type, is_ptr, struct_name, nb_ptr*/);
      /*if (a != nullptr){
        return a;
      }*/
    } else {
      Log::Info() << "Infering type (CurTok : " << CurTok << ")" << "\n";
      infer_type = true;
    }
    if (is_array){
        cpoint_type.is_array = is_array;
        cpoint_type.nb_element = from_val_to_int(index->clone()->codegen());;
    }
    // Read the optional initializer.
    std::unique_ptr<ExprAST> Init = nullptr;
    if (CurTok == '=') {
      getNextToken(); // eat the '='.

      Init = ParseExpression();
      if (!Init)
        return nullptr;
    }
    if (infer_type && Init == nullptr){
        Log::Info() << "Missing type declaration and default value to do type inference. Type defaults to double" << "\n";
        cpoint_type.type = double_type;
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
  return std::make_unique<VarExprAST>(std::move(VarNames)/*, std::move(Body)*/, cpoint_type, std::move(index), infer_type);
}
