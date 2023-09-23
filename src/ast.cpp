#include <map>
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
extern std::map<std::string, std::unique_ptr<NamedValue>> NamedValues;
extern std::map<std::string, std::unique_ptr<GlobalVariableValue>> GlobalVariables;
extern std::map<std::string, std::unique_ptr<EnumDeclaration>> EnumDeclarations;
//extern bool std_mode;
//extern bool gc_mode;
extern std::unique_ptr<Module> TheModule;
extern std::vector<std::string> types;
extern std::vector</*std::string*/ Cpoint_Type> typeDefTable;
extern std::map<std::string, std::unique_ptr<TemplateProto>> TemplateProtos;
extern std::map<std::string, std::unique_ptr<StructDeclar>> TemplateStructDeclars;
extern std::vector<std::string> modulesNamesContext;
extern std::pair<std::string, /*std::string*/ Cpoint_Type> TypeTemplateCallCodegen;
extern std::vector<std::unique_ptr<TemplateStructCreation>> StructTemplatesToGenerate;
extern std::string TypeTemplateCallAst;
extern std::map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;

Cpoint_Type ParseTypeDeclaration(bool eat_token);

bool is_comment = false;

extern void HandleComment();
extern std::string filename;

bool is_template_parsing_definition = false;
bool is_template_parsing_struct = false;

Source_location emptyLoc = {0, 0, true, ""};

std::unique_ptr<ExprAST> StructMemberExprAST::clone(){
    std::vector<std::unique_ptr<ExprAST>> ArgsCloned;
    if (!ArgsCloned.empty()){
      for (int i = 0; i < ArgsCloned.size(); i++){
        ArgsCloned.push_back(Args.at(i)->clone());
      }
    }
    return std::make_unique<StructMemberExprAST>(StructName, MemberName, is_function_call, std::move(ArgsCloned));
}

std::unique_ptr<ExprAST> ConstantArrayExprAST::clone(){
    std::vector<std::unique_ptr<ExprAST>> ArrayMembersCloned;
    if (!ArrayMembers.empty()){
      for (int i = 0; i < ArrayMembers.size(); i++){
        ArrayMembersCloned.push_back(ArrayMembers.at(i)->clone());
      }
    }
    return std::make_unique<ConstantArrayExprAST>(std::move(ArrayMembersCloned));
}

std::unique_ptr<ExprAST> ConstantStructExprAST::clone(){
    std::vector<std::unique_ptr<ExprAST>> StructMembersCloned;
    if (!StructMembers.empty()){
      for (int i = 0; i < StructMembers.size(); i++){
        StructMembersCloned.push_back(StructMembers.at(i)->clone());
      }
    }
    return std::make_unique<ConstantStructExprAST>(struct_name, std::move(StructMembersCloned));
}

std::unique_ptr<ExprAST> EnumCreation::clone(){
    std::unique_ptr<ExprAST> valueCloned = nullptr;
    if (value){
        valueCloned = value->clone();
    }
    return std::make_unique<EnumCreation>(EnumVarName, EnumMemberName, std::move(valueCloned));
}

std::unique_ptr<ExprAST> CallExprAST::clone(){
    std::vector<std::unique_ptr<ExprAST>> ArgsCloned;
    if (!Args.empty()){
      for (int i = 0; i < Args.size(); i++){
        ArgsCloned.push_back(Args.at(i)->clone());
      }
    }
    return std::make_unique<CallExprAST>(ExprAST::loc, Callee, std::move(ArgsCloned), template_passed_type);
}

std::unique_ptr<ExprAST> VarExprAST::clone(){
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

std::unique_ptr<ExprAST> RedeclarationExprAST::clone(){
    std::unique_ptr<ExprAST> indexCloned = nullptr;
    if (index != nullptr){
      indexCloned = index->clone();
    }
    return std::make_unique<RedeclarationExprAST>(VariableName, Val->clone(), member, std::move(indexCloned));
}

std::unique_ptr<FunctionAST> FunctionAST::clone(){
    std::vector<std::unique_ptr<ExprAST>> BodyClone;
    for (int i = 0; i < Body.size(); i++){
      BodyClone.push_back(Body.at(i)->clone());
    }
    return std::make_unique<FunctionAST>(Proto->clone(), std::move(BodyClone));
}

std::unique_ptr<StructDeclarAST> StructDeclarAST::clone(){
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
    return std::make_unique<EnumMember>(Name, contains_value, std::move(TypeCloned));
}

std::unique_ptr<EnumDeclarAST> EnumDeclarAST::clone(){
    std::vector<std::unique_ptr<EnumMember>> EnumMembersCloned;
    if (!EnumMembers.empty()){
        for (int i = 0; i < EnumMembers.size(); i++){
            EnumMembersCloned.push_back(EnumMembers.at(i)->clone());
        }
    }
    return std::make_unique<EnumDeclarAST>(Name, enum_member_contain_type, std::move(EnumMembersCloned));
}

std::unique_ptr<TestAST> TestAST::clone(){
    std::vector<std::unique_ptr<ExprAST>> BodyCloned;
    if (!Body.empty()){
      for (int i = 0; i < Body.size(); i++){
        BodyCloned.push_back(Body.at(i)->clone());
      }
    }
    return std::make_unique<TestAST>(description, std::move(BodyCloned));
}

std::unique_ptr<ExprAST> IfExprAST::clone(){
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

std::unique_ptr<ExprAST> ForExprAST::clone(){
    std::vector<std::unique_ptr<ExprAST>> BodyCloned;
    if (!Body.empty()){
      for (int i = 0; i < Body.size(); i++){
        BodyCloned.push_back(Body.at(i)->clone());
      }
    }
    return std::make_unique<ForExprAST>(VarName, Start->clone(), End->clone(), Step->clone(), std::move(BodyCloned));
}

std::unique_ptr<ExprAST> WhileExprAST::clone(){
    std::vector<std::unique_ptr<ExprAST>> BodyCloned;
    if (!Body.empty()){
      for (int i = 0; i < Body.size(); i++){
        BodyCloned.push_back(Body.at(i)->clone());
      }
    }
    return std::make_unique<WhileExprAST>(Cond->clone(), std::move(BodyCloned));
}

std::unique_ptr<ExprAST> LoopExprAST::clone(){
    std::vector<std::unique_ptr<ExprAST>> BodyCloned;
    if (!Body.empty()){
      for (int i = 0; i < Body.size(); i++){
        BodyCloned.push_back(Body.at(i)->clone());
      }
    }
    return std::make_unique<LoopExprAST>(VarName, Array->clone(), std::move(BodyCloned), is_infinite_loop);
}

std::unique_ptr<matchCase> matchCase::clone(){
    std::vector<std::unique_ptr<ExprAST>> BodyCloned;
    std::unique_ptr<ExprAST> exprCloned;
    if (!Body.empty()){
      for (int i = 0; i < Body.size(); i++){
        BodyCloned.push_back(Body.at(i)->clone());
      }
    } 
    if (expr){
        exprCloned = expr->clone();
    }
    return std::make_unique<matchCase>(std::move(exprCloned), enum_name, enum_member, var_name, is_underscore, std::move(BodyCloned));
}

std::unique_ptr<ExprAST> MatchExprAST::clone(){
    std::vector<std::unique_ptr<matchCase>> matchCasesCloned;
    if (!matchCases.empty()){
        for (int i = 0; i < matchCases.size(); i++){
            matchCasesCloned.push_back(matchCases.at(i)->clone());
        }
    }
    return std::make_unique<MatchExprAST>(matchVar, std::move(matchCasesCloned));
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
  bool is_array = false;
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
#if ARRAY_MEMBER_OPERATOR_IMPL == 0
  if (CurTok == '['){
    getNextToken();
    indexAST = ParseExpression();
    if (!indexAST){
      return LogError("Couldn't find index for array %s", IdName.c_str()); 
    }
    if (CurTok != ']'){
      return LogError("Missing ']'"); 
    }
    getNextToken();
    is_array = true;
  }
#endif
  if (CurTok == '.'){
    Log::Info() << "Struct member found" << "\n";
    getNextToken();
    member = IdentifierStr;
    getNextToken();
  }
  if (CurTok == '='){
    Log::Info() << "IdName " << IdName << "\n";
    Log::Info() << "RedeclarationExpr Parsing" << "\n";
    Log::Info() << "verify if " << IdName << "is in NamedValues : " << (int)(NamedValues[IdName] == nullptr) << "\n";
    if (member != ""){
    if (GlobalVariables[IdName] == nullptr && NamedValues[IdName] == nullptr && IdName != "self") {
      return LogError("Couldn't find variable %s when redeclarating it", IdName.c_str());
    }
    }
    getNextToken();
    auto V = ParseExpression();
    if (!V){
      return nullptr;
    }
    return std::make_unique<RedeclarationExprAST>(IdName, std::move(V), member, std::move(indexAST));
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
  if (member != ""){
    bool is_function_call_member = false;
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok == '('){
      Log::Info() << "call struct member function" << "\n";
      is_function_call_member = true;
      getNextToken();
      auto ret = ParseFunctionArgs(Args);
      if (!ret){
        return nullptr;
      }

    // Eat the ')'.
    getNextToken();
    }
    Log::Info() << "Struct member returned" << "\n";
    // make the struct members detection recursive : for example with a.b.c or in the linked list code
    if (NamedValues[IdName] != nullptr && NamedValues[IdName]->type.is_enum){
        // get Enum member (maybe not do it and replace with match)
    }
    if (!is_function_call_member && NamedValues[IdName] != nullptr && NamedValues[IdName]->type.is_union){
        return std::make_unique<UnionMemberExprAST>(IdName, member);
    }
    return std::make_unique<StructMemberExprAST>(IdName, member, is_function_call_member, std::move(Args));
  }
  if (CurTok != '(' && CurTok != /*'<'*/ '~'){ // Simple variable ref.
#if ARRAY_MEMBER_OPERATOR_IMPL == 0
    if (is_array){
      Log::Info() << "Array member returned" << "\n";
      return std::make_unique<ArrayMemberExprAST>(IdName, std::move(indexAST));
    }
#endif
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
  }
  // Call.
  /*std::string*/ Cpoint_Type template_passed_type = /*""*/ Cpoint_Type(double_type);
  if (CurTok == '~'){
    getNextToken();
    template_passed_type = /*IdentifierStr*/ ParseTypeDeclaration(false);
    //getNextToken();
    if (CurTok != '~'){
      return LogError("expected '~' not found");
    }
    getNextToken();
  }
  if (CurTok != '('){
    return LogError("missing '(' when calling function");
  }
  getNextToken();  // eat (
  std::vector<std::unique_ptr<ExprAST>> Args;
  auto ret = ParseFunctionArgs(Args);
  if (!ret){
    return nullptr;
  }
  /*if (CurTok != ')') {
    while (1) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')')
        break;

      if (CurTok != ',')
        return LogError("Expected ')' or ',' in argument list");
      getNextToken();
    }
  }*/

  // Eat the ')'.
  getNextToken();

  return std::make_unique<CallExprAST>(IdLoc, IdName, std::move(Args), template_passed_type);
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  Log::Info() << "PARSING BINOP" << "\n";
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
    if (TokPrec < ExprPrec)
      return LHS;
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
    /*if (CurTok == '=' || CurTok == '|'){
      BinOp += (char)CurTok;
      getNextToken();
    }*/
    Log::Info() << "BinOP : " << BinOp << "\n";

    // Parse the primary expression after the binary operator.
    //auto RHS = ParsePrimary();
    auto RHS = ParseUnary();
    if (!RHS)
      return nullptr;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = -1;
    if (CurTok != tok_op_multi_char){
    NextPrec = GetTokPrecedence();
    } else {
      NextPrec = getTokPrecedenceMultiChar(OpStringMultiChar);
    }
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }
#if ARRAY_MEMBER_OPERATOR_IMPL
    if (BinOp == "["){
        // TODO add better verification for end ']'
        getNextToken(); // pass ']'
    }
#endif
    // Merge LHS/RHS.
    LHS =
        std::make_unique<BinaryExprAST>(BinLoc, BinOp, std::move(LHS), std::move(RHS));
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
  }
}

std::unique_ptr<StringExprAST> get_filename_tok(){
    std::string filename_without_temp = filename;
    int temp_pos;
    if ((temp_pos = filename.rfind(".temp")) != std::string::npos){
        filename_without_temp = filename.substr(0, temp_pos);
    }
    return std::make_unique<StringExprAST>(filename_without_temp);
}

std::unique_ptr<StringExprAST> stringify_macro(std::unique_ptr<ExprAST> expr){
    return std::make_unique<StringExprAST>(expr->to_string());
}

std::unique_ptr<ExprAST> ParseMacroCall(){
    Log::Info() << "Parsing macro call" << "\n";
    getNextToken();
    std::string function_name = IdentifierStr;
    getNextToken();
    if (CurTok != '('){
        return LogError("missing '(' in the call of a macro function");
    }
    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> ArgsMacro;
    auto ret = ParseFunctionArgs(ArgsMacro);
    if (!ret){
        return nullptr;
    }
    getNextToken();
    if (function_name == "file"){
        return get_filename_tok();
    } else if (function_name == "expect"){
        std::vector<std::unique_ptr<ExprAST>> Args;
        std::vector<std::unique_ptr<ExprAST>> ArgsEmpty;
        if (ArgsMacro.size() != 1){
            return LogError("Wrong number of args for %s macro function call : expected %d, got %d", "expect", 1, ArgsMacro.size());
        }
        Args.push_back(ArgsMacro.at(0)->clone());
        Args.push_back(get_filename_tok());
        Args.push_back(std::make_unique<CallExprAST>(emptyLoc, "cpoint_internal_get_function_name", std::move(ArgsEmpty), Cpoint_Type(double_type)));
        Args.push_back(stringify_macro(ArgsMacro.at(0)->clone()));
        return std::make_unique<CallExprAST>(emptyLoc, "expectxplusexpr", std::move(Args), Cpoint_Type(double_type));
    } else if (function_name == "panic"){
        std::vector<std::unique_ptr<ExprAST>> Args;
        std::vector<std::unique_ptr<ExprAST>> ArgsEmpty;
        if (ArgsMacro.size() != 1){
            return LogError("Wrong number of args for %s macro function call : expected %d, got %d", "expect", 1, ArgsMacro.size());
        }
        Args.push_back(ArgsMacro.at(0)->clone());
        Args.push_back(get_filename_tok());
        Args.push_back(std::make_unique<CallExprAST>(emptyLoc, "cpoint_internal_get_function_name", std::move(ArgsEmpty), Cpoint_Type(double_type)));
        return std::make_unique<CallExprAST>(emptyLoc, "panicx", std::move(Args), Cpoint_Type(double_type));
    } else if (function_name == "stringify"){
        if (ArgsMacro.size() != 1){
            return LogError("Wrong number of args for %s macro function call : expected %d, got %d", "stringify", 1, ArgsMacro.size());
        } 
        return stringify_macro(std::move(ArgsMacro.at(0)));
    } else if (function_name == "concat"){
        std::string concatenated_str = "";
        for (int i = 0; i < ArgsMacro.size(); i++){
            if (dynamic_cast<StringExprAST*>(ArgsMacro.at(i).get())){
                auto s = dynamic_cast<StringExprAST*>(ArgsMacro.at(i).get());
                concatenated_str += s->str;
            } else {
                concatenated_str += ArgsMacro.at(i)->to_string();
            }
        }
        return std::make_unique<StringExprAST>(concatenated_str);
    } else if (function_name == "line"){
        return std::make_unique<StringExprAST>(Comp_context->line);
    } else if (function_name == "line_nb"){
        return std::make_unique<NumberExprAST>((double)Comp_context->curloc.line_nb);
    } else if (function_name == "column_nb"){
        return std::make_unique<NumberExprAST>((double)Comp_context->curloc.col_nb);
    } else if (function_name == "time"){
        std::string time_str;
        char* time_str_c = (char*)malloc(sizeof(char) * 21);
        std::time_t current_time;
        std::tm* time_info;
        time(&current_time);
        time_info = localtime(&current_time);
        strftime(time_str_c, 21, "%H:%M:%S", time_info);
        time_str = time_str_c;
        free(time_str_c);
        return std::make_unique<StringExprAST>(time_str);
    } else if (function_name == "env"){
        if (ArgsMacro.size() != 1){
            return LogError("Wrong number of args for %s macro function call : expected %d, got %d", "env", 1, ArgsMacro.size());
        }
        StringExprAST* str = nullptr;
        if (dynamic_cast<StringExprAST*>(ArgsMacro.at(0).get())){
            str = dynamic_cast<StringExprAST*>(ArgsMacro.at(0).get());
        } else {
            return LogError("Wrong type of args for %s macro function call : expected a string", "env");
        }
        std::string env_name = str->str;
        auto path_val = std::getenv(env_name.c_str());
        if (path_val == nullptr){
            return LogError("Env variable %s doesn't exist for %s macro function call", env_name.c_str(), "env");
        }
        return std::make_unique<StringExprAST>((std::string)path_val);
    }
    return LogError("unknown function macro called : %s", function_name.c_str());
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
  //std::string struct_template_name;
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
        //struct_template_name = IdentifierStr;
        //getNextToken();
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
    if (CurTok == tok_ptr){
      is_ptr = true;
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
  if (CurTok != ')') {
    while (1) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')')
        break;

      if (CurTok != ',')
        return LogError("Expected ')' or ',' in argument list");
      getNextToken();
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
  /*if (CurTok != tok_identifier)
    return LogErrorP("Expected function name in prototype");*/

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
    ArgNames.push_back(std::make_pair("argc", Cpoint_Type(double_type, false)));
    //ArgNames.push_back(std::make_pair("argv",  Cpoint_Type(argv_type, false)));
    ArgNames.push_back(std::make_pair("argv",  Cpoint_Type(i8_type, true, 2)));
  } else {
  getNextToken();
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
          return LogErrorP("Expected ')' or ',' in argument list");
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
  if (CurTok != '{')
    return LogErrorS("Expected '{' in Struct");
  getNextToken();
  while ((CurTok == tok_var || CurTok == tok_func || CurTok == tok_extern) && CurTok != '}'){
    Log::Info() << "Curtok in struct parsing : " << CurTok << "\n";
    if (CurTok == tok_var){
    auto exprAST = ParseVarExpr();
    if (!exprAST){
        return nullptr;
    }
    VarExprAST* varExprAST = dynamic_cast<VarExprAST*>(exprAST.get());
    if (varExprAST == nullptr){
      return LogErrorS("Error in struct declaration vars");
    }
    std::unique_ptr<VarExprAST> declar;
    exprAST.release();
    declar.reset(varExprAST);
    if (!declar){return nullptr;}
    VarList.push_back(std::move(declar));
    //getNextToken();
    //std::cout << "currTok IN LOOP : " << CurTok << std::endl;
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
    while (CurTok == tok_func && CurTok != '}'){
        auto funcAST = ParseDefinition();
        FunctionAST* functionAST = dynamic_cast<FunctionAST*>(funcAST.get());
        if (functionAST == nullptr){
            return LogErrorMembers("Error in members declaration funcs");
        }
        std::unique_ptr<FunctionAST> declar;
        funcAST.release();
        declar.reset(functionAST);
        if (declar == nullptr){return nullptr;}
        Functions.push_back(std::move(declar));
    }
    if (CurTok != '}'){
        return LogErrorMembers("Expected '}' in members");
    }
    getNextToken();  // eat '}'.
    return std::make_unique<MembersDeclarAST>(members_name, members_for, std::move(Functions));
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
        std::unique_ptr<Cpoint_Type> Type = nullptr;
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
        EnumMembers.push_back(std::make_unique<EnumMember>(memberName, contains_value, std::move(Type)));
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
  /*if (Proto->has_template){
  TypeTemplatCallAst = Proto->template_name;
  }*/
  for (int i = 0; i < Proto->Args.size(); i++){
    Log::Info() << "args added to NamedValues when doing ast : " << Proto->Args.at(i).first << "\n";
    NamedValues[Proto->Args.at(i).first] = std::make_unique<NamedValue>(nullptr,Proto->Args.at(i).second);
  }
  StructTemplatesToGenerate.clear();
  Log::Info() << "cleared StructTemplatesToGenerate" << "\n";
  std::vector<std::unique_ptr<ExprAST>> Body;
  if (Comp_context->std_mode && Proto->Name == "main" && Comp_context->gc_mode){
  std::vector<std::unique_ptr<ExprAST>> Args_gc_init;
  auto E_gc_init = std::make_unique<CallExprAST>(emptyLoc, "gc_init", std::move(Args_gc_init), Cpoint_Type(double_type));
  Body.push_back(std::move(E_gc_init));
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
  std::string Name = IdentifierStr;
  auto addr = std::make_unique<AddrExprAST>(Name);
  getNextToken();
  return addr;
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
  getNextToken(); // eat '{'
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
  Log::Info() << "PARSE UNARY" << "\n";
  // If the current token is not an operator, it must be a primary expr.
  if (!isascii(CurTok) || CurTok == '(' || CurTok == ',' || CurTok == '{' || CurTok == ':' || CurTok == tok_string || CurTok == tok_false || CurTok == tok_true || CurTok == '[' || CurTok == '#')
    return ParsePrimary();

  // If this is a unary operator, read it.
  Log::Info() << "Is an unary operator " << CurTok << "\n";
  int Opc = CurTok;
  getNextToken();
  if (auto Operand = ParseUnary())
    return std::make_unique<UnaryExprAST>(Opc, std::move(Operand));
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
  return std::make_unique<GlobalVariableAST>(Name, is_const, is_extern, cpoint_type, std::move(Init));
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

  return std::make_unique<ForExprAST>(IdName, std::move(Start),
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
        //cpoint_type.nb_element = from_val_to_constant_infer(index->clone()->codegen());
        cpoint_type.nb_element = dyn_cast<ConstantFP>(index->clone()->codegen())->getValue().convertToDouble();
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
