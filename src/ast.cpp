#include <map>
#include <iostream>
#include <utility>
#include <cstdarg>
#include "ast.h"
#include "lexer.h"
#include "types.h"
#include "errors.h"
#include "log.h"
#include "codegen.h"

extern double NumVal;
extern int CurTok;
extern std::string strStatic;
extern int charStatic;
extern std::string IdentifierStr;
extern int return_status;
extern std::string strPosArray;
extern std::string OpStringMultiChar;
extern int posArrayNb;
bool isInStruct = false;;
extern std::unique_ptr<Compiler_context> Comp_context;
extern std::map<std::string, std::unique_ptr<NamedValue>> NamedValues;
extern std::map<std::string, std::unique_ptr<GlobalVariableValue>> GlobalVariables;
extern bool std_mode;
extern bool gc_mode;
extern std::unique_ptr<Module> TheModule;
extern std::vector<std::string> types;
extern std::vector<std::string> typeDefTable;
extern std::map<std::string, std::unique_ptr<TemplateType>> TemplateTypes;

std::unique_ptr<ExprAST> vLogError(const char* Str, va_list args){
  vlogErrorExit(std::make_unique<Compiler_context>(*Comp_context), Str, args); // copy comp_context and not move it because it will be used multiple times
  return_status = 1;
  return nullptr;
}

std::unique_ptr<ExprAST> LogError(const char *Str, ...) {
  va_list args;
  va_start(args, Str);
  //fprintf(stderr, "LogError: %s\n", Str);
  vLogError(Str, args);
  va_end(args);
  return nullptr;
}


std::unique_ptr<PrototypeAST> LogErrorP(const char *Str, ...) {
  va_list args;
  va_start(args, Str);
  vLogError(Str, args);
  va_end(args);
  return nullptr;
}

std::unique_ptr<ReturnAST> LogErrorR(const char *Str, ...) {
  va_list args;
  va_start(args, Str);
  vLogError(Str, args);
  Log::Info() << "token : " << CurTok << "\n";
  va_end(args);
  return nullptr;
}


std::unique_ptr<StructDeclarAST> LogErrorS(const char *Str, ...) {
  va_list args;
  va_start(args, Str);
  vLogError(Str, args);
  Log::Info() << "token : " << CurTok << "\n";
  va_end(args);
  return nullptr;
}

std::unique_ptr<ClassDeclarAST> LogErrorC(const char *Str, ...) {
  va_list args;
  va_start(args, Str);
  vLogError(Str, args);
  Log::Info() << "token : " << CurTok << "\n";
  va_end(args);
  return nullptr;
}

std::unique_ptr<FunctionAST> LogErrorF(const char *Str, ...) {
  va_list args;
  va_start(args, Str);
  vLogError(Str, args);
  va_end(args);
  return nullptr;
}

std::unique_ptr<GlobalVariableAST> LogErrorG(const char *Str, ...) {
  va_list args;
  va_start(args, Str);
  vLogError(Str, args);
  va_end(args);
  return nullptr;
}

std::unique_ptr<LoopExprAST> LogErrorL(const char* Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args);
  va_end(args);
  return nullptr;
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

static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;
  Log::Info() << "Parse Identifier Str" << "\n";
  getNextToken();  // eat identifier.
  std::string member = "";
  std::unique_ptr<ExprAST> indexAST = nullptr;
  bool is_array = false;
  if (CurTok == '['){
    getNextToken();
    indexAST = ParseExpression();
    if (!indexAST){
      return LogError("Couldn't find index for array %s", IdName.c_str()); 
    }
    if (CurTok != ']'){
      return LogError("Missing '['"); 
    }
    getNextToken();
    is_array = true;
  }
  if (CurTok == '.'){
    Log::Info() << "Struct member found" << "\n";
    getNextToken();
    member = IdentifierStr;
    getNextToken();
  }
  if (CurTok == '='){
    Log::Info() << "IdName " << IdName << "\n";
    Log::Info() << "RedeclarationExpr Parsing" << "\n";
    if (GlobalVariables[IdName] == nullptr && NamedValues[IdName] == nullptr) {
      return LogError("Couldn't find variable %s when redeclarating it", IdName.c_str());
    }
    getNextToken();
    auto V = ParseExpression();
    return std::make_unique<RedeclarationExprAST>(IdName, std::move(V), member, std::move(indexAST));
  }
  if (CurTok != '('){ // Simple variable ref.
    if (member != ""){
      Log::Info() << "Struct member returned" << "\n";
      return std::make_unique<StructMemberExprAST>(IdName, member);
    }
    if (is_array){
      Log::Info() << "Array member returned" << "\n";
      return std::make_unique<ArrayMemberExprAST>(IdName, std::move(indexAST));
    }
    /*for (int i = 0; i < IdName.length(); i++){
      if (IdName.at(i) == '['){
        if (IdName.back() != ']')
          return LogError("Couldn't find ]");
        std::string VarName = IdName.substr(0, i);
        std::string pos_str = "";
        for (int j = i + 1; j < IdName.length(); j++){
          if (IdName.at(j) == ']'){
            break;
          } else {
            pos_str += IdName.at(j);
          }
        }
        //std::string pos_str = IdName.substr(i+1, IdName.length()-2);
        Log::Info() << "NAME LENGTH " << IdName.length() << "\n";
        Log::Info() << "ARRAY MEMBER " << VarName << " " << pos_str << "\n";
        int pos = std::stoi(pos_str);
        return std::make_unique<ArrayMemberExprAST>(VarName, pos);
      }
    }*/
    Log::Info() << "VariableExprAST" << "\n";
    std::unique_ptr<Cpoint_Type> type;
    if (NamedValues[IdName] == nullptr && GlobalVariables[IdName] == nullptr){
      type = std::make_unique<Cpoint_Type>(double_type);
    } else if (GlobalVariables[IdName] != nullptr){
      type = std::make_unique<Cpoint_Type>(GlobalVariables[IdName]->type);
    } else  {
      type = std::make_unique<Cpoint_Type>(NamedValues[IdName]->type);
    }
    return std::make_unique<VariableExprAST>(IdName, *type);
  }

  // Call.
  getNextToken();  // eat (
  std::vector<std::unique_ptr<ExprAST>> Args;
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

  // Eat the ')'.
  getNextToken();

  return std::make_unique<CallExprAST>(IdName, std::move(Args));
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
    Log::Info() << "CurTok before if : " << CurTok << "\n";
    //if (CurTok != tok_op_multi_char){
    if (TokPrec < ExprPrec)
      return LHS;
    //}
    //}

    // Okay, we know this is a binop.
    std::string BinOp = "";
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

    // Merge LHS/RHS.
    LHS =
        std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
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
  case tok_if:
    return ParseIfExpr();
  case tok_return:
    return ParseReturn();
  case tok_for:
    return ParseForExpr();
  case tok_var:
    return ParseVarExpr();
  case tok_string:
    Log::Info() << "tok string found" << "\n";
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
  case tok_goto:
    return ParseGotoExpr();
  case tok_label:
    return ParseLabelExpr();
  case tok_break:
    return ParseBreakExpr();
  }
}

std::unique_ptr<ExprAST> ParseTypeDeclaration(int* type, bool* is_ptr, std::string& struct_Name, std::string& class_Name, int& nb_ptr , bool eat_token = true){
  Log::Info() << "type declaration found" << "\n";
  if (eat_token){
  getNextToken(); // eat the ':'
  }
  if (CurTok != tok_identifier && CurTok != tok_struct && CurTok != tok_class)
    return LogError("expected identifier after var in type declaration");
  if (CurTok == tok_struct){
    getNextToken();
    struct_Name = IdentifierStr;
    Log::Info() << "struct_Name " << struct_Name << "\n";
    getNextToken();
    if (CurTok == tok_ptr){
      *is_ptr = true;
      getNextToken();
    }
  } else if (CurTok == tok_class){
    getNextToken();
    class_Name = IdentifierStr;
    getNextToken();
    if (CurTok == tok_ptr){
      *is_ptr = true;
      getNextToken();
    }
  } else if (is_type(IdentifierStr)){
    *type = get_type(IdentifierStr);
    Log::Info() << "Variable type : " << *type << "\n";
    getNextToken();
    while (CurTok == tok_ptr){
      *is_ptr = true;
      nb_ptr++;
      Log::Info() << "Type Declaration ptr added : " << nb_ptr << "\n";
      getNextToken();
    }
  } else {
    return LogError("wrong type %s found", IdentifierStr.c_str());
  }
  return nullptr;
}

std::unique_ptr<ExprAST> ParseBool(bool bool_value){
  return std::make_unique<BoolExprAST>(bool_value);
}

static std::unique_ptr<PrototypeAST> ParsePrototype() {
  /*if (CurTok != tok_identifier)
    return LogErrorP("Expected function name in prototype");*/

  std::string FnName;
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
  if (CurTok == '<'){
    getNextToken();
    template_name = IdentifierStr;
    getNextToken();
    if (CurTok != '>'){
      return LogErrorP("Missing '>' in template declaration");
    }
    getNextToken();
    has_template = true;
    Log::Info() << "Found template" << "\n";
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
    ArgNames.push_back(std::make_pair("argv",  Cpoint_Type(argv_type, false)));
  } else {
  getNextToken();
  if (CurTok != '('){
    int arg_nb = 0;
    while (1){
      if (CurTok == tok_format){
      Log::Info() << "Found Variable number of args" << "\n";
      is_variable_number_args = true;
      //ArgNames.push_back(std::make_pair("...", Cpoint_Type(double_type)));
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
      int type = double_type;
      bool is_ptr = false;
      std::string temp_struct;
      std::string temp_class;
      int temp_nb = -1;
      if (CurTok == ':'){
        ParseTypeDeclaration(&type, &is_ptr, temp_struct, temp_class, temp_nb);
      }
      arg_nb++;
      ArgNames.push_back(std::make_pair(ArgName, Cpoint_Type(type, is_ptr, false, 0, temp_struct != "", temp_struct, false, "", temp_nb)));
      }
    }
  }

  }

  // success.
  getNextToken();  // eat ')'.
  if (Kind && ArgNames.size() != Kind)
    return LogErrorP("Invalid number of operands for operator : %d args but %d expected", ArgNames.size(), Kind);
  Log::Info() << "Tok : " << CurTok << "\n";
  int type = -1;
  bool is_ptr = false;
  std::string struct_name = "";
  std::string class_name = "";
  int nb_ptr = 0;
  if (CurTok == tok_identifier || CurTok == tok_struct || CurTok == tok_class){
    Log::Info() << "Tok type : " << CurTok << "\n";
    Log::Info() << "type : " << IdentifierStr << "\n";
    ParseTypeDeclaration(&type, &is_ptr, struct_name, class_name, nb_ptr, false);
    //type = get_type(IdentifierStr);
    //getNextToken(); // eat type
  }
  bool is_struct = struct_name != "";
  bool is_class = class_name != "";
  Cpoint_Type cpoint_type = Cpoint_Type(type, is_ptr, false, 0, is_struct, struct_name, is_class, class_name, nb_ptr);
  /*if (struct_name != ""){
  cpoint_type = std::make_unique<Cpoint_Type>(0, is_ptr, false, 0, true, struct_name, nb_ptr);
  } else {
  cpoint_type = std::make_unique<Cpoint_Type>(type, is_ptr, false, 0, false, "", nb_ptr);
  }*/
  return std::make_unique<PrototypeAST>(FnName, ArgNames, cpoint_type, Kind != 0, BinaryPrecedence, is_variable_number_args, has_template, template_name);
}

std::unique_ptr<StructDeclarAST> ParseStruct(){
  isInStruct = true;
  std::vector<std::unique_ptr<VarExprAST>> VarList;
  getNextToken(); // eat struct
  std::string structName = IdentifierStr;
  Log::Info() << "Struct Name : " << structName << "\n";
  getNextToken();
  if (CurTok != '{')
    return LogErrorS("Expected '{' in prototype");
  getNextToken();
  while (CurTok == tok_var){
    auto exprAST = ParseVarExpr();
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
  }
  if (CurTok != '}'){
    return LogErrorS("Expected '}' in prototype");
  }
  getNextToken();  // eat '}'.
  isInStruct = false;
  return std::make_unique<StructDeclarAST>(structName, std::move(VarList));
}

std::unique_ptr<ClassDeclarAST> ParseClass(){
  isInStruct = true;
  int i = 0;
  std::vector<std::unique_ptr<VarExprAST>> VarList;
  std::vector<std::unique_ptr<FunctionAST>> Functions;
  getNextToken(); // eat class
  std::string className = IdentifierStr;
  Log::Info() << "Class Name : " << className << "\n";
  getNextToken();
  if (CurTok != '{')
    return LogErrorC("Expected '{' in prototype");
  getNextToken();
  while ((CurTok == tok_var || /*CurTok == tok_class ||*/ CurTok == tok_func) && CurTok != '}'){
    Log::Info() << "Curtok in class parsing : " << CurTok << "\n";
    if (CurTok == tok_var){
    auto exprAST = ParseVarExpr();
    VarExprAST* varExprAST = dynamic_cast<VarExprAST*>(exprAST.get());
    if (varExprAST == nullptr){
      return LogErrorC("Error in class declaration vars");
    }
    std::unique_ptr<VarExprAST> declar;
    exprAST.release();
    declar.reset(varExprAST);
    if (!declar){return nullptr;}
    VarList.push_back(std::move(declar));
    //getNextToken();
    //std::cout << "currTok IN LOOP : " << CurTok << std::endl;
    } else if (CurTok == tok_func){
      Log::Info() << "function found in class" << "\n";
      auto funcAST = ParseDefinition();
      Log::Info() << "AFTER ParseDefinition" << "\n";
      FunctionAST* functionAST = dynamic_cast<FunctionAST*>(funcAST.get());
      if (functionAST == nullptr){
      return LogErrorC("Error in class declaration funcs");
      }
      std::unique_ptr<FunctionAST> declar;
      funcAST.release();
      declar.reset(functionAST);
      if (declar == nullptr){return nullptr;}
      Functions.push_back(std::move(declar));
      Log::Info() << "Function Number in class : " << i+1 << "\n";
      i++;
    }
  }
  Log::Info() << "CurTok : " << CurTok << "\n";
  if (CurTok != '}'){
    return LogErrorC("Expected '}' in prototype");
  }
  getNextToken();  // eat '}'.
  isInStruct = false;
  Log::Info() << "RETURNED ClassDeclarAST" << "\n";
  return std::make_unique<ClassDeclarAST>(className, std::move(VarList), std::move(Functions));
}

std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken();  // eat func.
  auto Proto = ParsePrototype();
  if (!Proto) return nullptr;
  auto argsCopy(Proto->Args);
  auto ProtoCopy = std::make_unique<PrototypeAST>(Proto->Name, argsCopy, Proto->cpoint_type, Proto->IsOperator, Proto->Precedence, Proto->is_variable_number_args, Proto->has_template, Proto->template_name);
  if (CurTok != '{'){
    LogErrorF("Expected '{' in function definition");
  }
  getNextToken();  // eat '{'
  std::vector<std::unique_ptr<ExprAST>> Body;
  if (std_mode && Proto->Name == "main" && gc_mode){
  std::vector<std::unique_ptr<ExprAST>> Args_gc_init;
  auto E_gc_init = std::make_unique<CallExprAST>("gc_init", std::move(Args_gc_init));
  Body.push_back(std::move(E_gc_init));
  }
  while (CurTok != '}'){
    auto E = ParseExpression();
    if (!E){
       return nullptr;
    }
    Body.push_back(std::move(E));
  }
  getNextToken(); // eat }
  Log::Info() << "end of function" << "\n";
  bool has_template = Proto->has_template;
  std::unique_ptr<FunctionAST> functionAST = std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
  if (has_template){
    std::string template_name = functionAST->Proto->template_name;
    Log::Info() << "Added template : " << template_name << "\n";
    TemplateTypes[template_name] = std::make_unique<TemplateType>(std::move(functionAST));
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
  getNextToken();
  std::string Name = IdentifierStr;
  auto Sizeof = std::make_unique<SizeofExprAST>(Name);
  Log::Info() << "after sizeof : " << Name << "\n";
  getNextToken();
  return Sizeof;
}

std::unique_ptr<ExprAST> ParseUnary() {
  Log::Info() << "PARSE UNARY" << "\n";
  // If the current token is not an operator, it must be a primary expr.
  if (!isascii(CurTok) || CurTok == '(' || CurTok == ',' || CurTok == '{' || CurTok == ':' || CurTok == tok_string)
    return ParsePrimary();

  // If this is a unary operator, read it.
  Log::Info() << "Is an unary operator " << CurTok << "\n";
  int Opc = CurTok;
  getNextToken();
  if (auto Operand = ParseUnary())
    return std::make_unique<UnaryExprAST>(Opc, std::move(Operand));
  return nullptr;
}

std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  Log::Info() << "ParseTopLevelExpr" << "\n";
  auto E = ParseExpression();
  if (!E){
    return nullptr;
  }
  auto Proto = std::make_unique<PrototypeAST>("main", std::vector<std::pair<std::string, Cpoint_Type>>(), Cpoint_Type(0));
  std::vector<std::unique_ptr<ExprAST>> Body;
  Body.push_back(std::move(E));
  return std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
}

std::unique_ptr<TypeDefAST> ParseTypeDef(){
  getNextToken(); // eat the 'type'
  std::string new_type = IdentifierStr;
  getNextToken();
  std::string value_type = IdentifierStr;
  getNextToken();
  types.push_back(new_type);
  typeDefTable.push_back(value_type);
  return std::make_unique<TypeDefAST>(new_type, value_type);
}

std::unique_ptr<GlobalVariableAST> ParseGlobalVariable(){
  getNextToken(); // eat the var.
  int type = -1;
  bool is_ptr = false;
  int nb_element = 0;
  bool is_array = false;
  std::string struct_name = "";
  std::string class_name = "";
  int nb_ptr = 0;
  bool is_const = false;
  if (IdentifierStr == "const"){
    is_const = true;
    getNextToken();
  }
  std::string Name = IdentifierStr;
  getNextToken(); // eat identifier.
  if (CurTok == ':'){
    auto a = ParseTypeDeclaration(&type, &is_ptr, struct_name, class_name, nb_ptr);
    if (a != nullptr){
      //return a;
    }
  }
  std::unique_ptr<ExprAST> Init = nullptr;
  if (CurTok == '=') {
  getNextToken(); // eat the '='.
  Init = ParseExpression();
  if (!Init)
    return nullptr;
  }
  std::unique_ptr<Cpoint_Type> cpoint_type;
  cpoint_type = std::make_unique<Cpoint_Type>(type, is_ptr, is_array, nb_element, false, "", false, "", nb_ptr);
  return std::make_unique<GlobalVariableAST>(Name, is_const, std::move(cpoint_type), std::move(Init));
}

std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken();  // eat the if.

  // condition.
  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;
  if (CurTok != '{'){
    Log::Info() << "CurTok : " << CurTok << "\n";
    return LogError("expected {");
  }
  /*if (CurTok != tok_then)
    return LogError("expected then");*/
  getNextToken();  // eat the {
  std::vector<std::unique_ptr<ExprAST>> Then;
  while (CurTok != '}'){
    auto E = ParseExpression();
    if (!E)
      return nullptr;
    Then.push_back(std::move(E));
  }
  Log::Info() << "CurTok : " << CurTok << "\n";
  /*if (CurTok != '}'){
    return LogError("expected }");
  }*/
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
  while (CurTok != '}'){
    auto E2 = ParseExpression();
    if (!E2)
      return nullptr;
    Else.push_back(std::move(E2));
  }
  /*if (CurTok != '}'){
    return LogError("expected }");
  }*/
  getNextToken();
  }
  }
  return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                      std::move(Else));
}


std::unique_ptr<ExprAST> ParseReturn(){
  getNextToken(); // eat the return
  Log::Info() << "CurTok return : " << CurTok << " " << NumVal << "\n";

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

std::unique_ptr<ExprAST> ParseLoopExpr(){
  getNextToken();  // eat the loop.
  std::vector<std::unique_ptr<ExprAST>> Body;
  if (CurTok == '{'){
    // infinite loop like in rust
    getNextToken();
    while (CurTok != '}'){
      auto E = ParseExpression();
      if (!E)
        return nullptr;
      Body.push_back(std::move(E));
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
    while (CurTok != '}'){
      auto E = ParseExpression();
      if (!E)
        return nullptr;
      Body.push_back(std::move(E));
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
  while (CurTok != '}'){
    auto E = ParseExpression();
    if (!E)
      return nullptr;
    Body.push_back(std::move(E));
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
  while (CurTok != '}'){
    auto E = ParseExpression();
    if (!E)
      return nullptr;
    Body.push_back(std::move(E));
  }
  /*if (CurTok != '}'){
    return LogError("expected } after for");
  }*/
  getNextToken();

  return std::make_unique<ForExprAST>(IdName, std::move(Start),
                                       std::move(End), std::move(Step),
                                       std::move(Body));
}

std::unique_ptr<ExprAST> ParseVarExpr() {
  getNextToken(); // eat the var.

  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
  int type = double_type;
  bool is_ptr = false;
  //int nb_element = 0;
  bool is_array = false;
  std::string struct_name = "";
  std::string class_name = "";
  int nb_ptr = 0;
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
    /*if (Name.find('[') != std::string::npos){
      is_array = true;
      std::string nb_element_str = "";
      std::string Var_Name ="";
      int i = 0;
      for (i = 0; i < Name.size(); i++){
        if (Name.at(i) == '['){
          break;
        } else {
          Var_Name += Name.at(i);
        }
      }
      for (int j = i + 1; j < Name.size(); j++){
        if (Name.at(j) == ']'){
          break;
        } else {
          nb_element_str += Name.at(j);
        }
      }
      nb_element = std::stoi(nb_element_str);
      Name = Var_Name;
    }*/
    infer_type = false;
    if (CurTok == ':'){
      auto a = ParseTypeDeclaration(&type, &is_ptr, struct_name, class_name, nb_ptr);
      if (a != nullptr){
        return a;
      }
    } else {
      infer_type = true;
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
        type = double_type;
    }
    VarNames.push_back(std::make_pair(Name, std::move(Init)));

    // End of var list, exit loop.
    if (CurTok != ',')
      break;
    getNextToken(); // eat the ','.

    if (CurTok != tok_identifier)
      return LogError("expected identifier list after var");
  }

  // At this point, we have to have 'in'.
  /*if (CurTok != tok_in)
    return LogError("expected 'in' keyword after 'var'");
  getNextToken(); // eat 'in'.

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;
  */
 //std::cout << "PARSED VARIABLES: " << VarNames.at(0).first << std::endl;
  std::unique_ptr<Cpoint_Type> cpoint_type;
  Log::Info() << "NB PTR : " << nb_ptr << "\n";
  bool is_struct = struct_name != "";
  bool is_class = class_name != "";
  cpoint_type = std::make_unique<Cpoint_Type>(type, is_ptr, is_array, 0 /*deprecated*/, is_struct, struct_name, is_class, class_name, nb_ptr);
  /*if (struct_name != ""){
  cpoint_type = std::make_unique<Cpoint_Type>(0, is_ptr, false, 0, true, struct_name, nb_ptr);
  } else {
  cpoint_type = std::make_unique<Cpoint_Type>(type, is_ptr, is_array, nb_element, false, "", nb_ptr);
  }*/
  NamedValues[VarNames.at(0).first] = std::make_unique<NamedValue>(nullptr, *cpoint_type);
  return std::make_unique<VarExprAST>(std::move(VarNames)/*, std::move(Body)*/, std::move(cpoint_type), std::move(index), infer_type);
}
