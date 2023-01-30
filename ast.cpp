#include <map>
#include <iostream>
#include <utility>
#include "ast.h"
#include "lexer.h"
#include "types.h"
#include "errors.h"
#include "log.h"
#include "codegen.h"

extern double NumVal;
extern int CurTok;
extern std::string strStatic;
extern std::string IdentifierStr;
extern int return_status;
extern std::string strPosArray;
extern int posArrayNb;
bool isInObject = false;;
extern std::unique_ptr<Compiler_context> Comp_context;
extern std::map<std::string, std::unique_ptr<NamedValue>> NamedValues;


std::unique_ptr<ExprAST> LogError(const char *Str) {
  //fprintf(stderr, "LogError: %s\n", Str);
  logErrorExit(std::move(Comp_context), Str);
  return_status = 1;
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}

std::unique_ptr<ReturnAST> LogErrorR(const char *Str) {
  LogError(Str);
  Log::Info() << "token : " << CurTok << "\n";
  return nullptr;
}


std::unique_ptr<ObjectDeclarAST> LogErrorO(const char *Str) {
  LogError(Str);
  Log::Info() << "token : " << CurTok << "\n";
  return nullptr;
}

std::unique_ptr<FunctionAST> LogErrorF(const char *Str) {
  LogError(Str);
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

  getNextToken();  // eat identifier.

  if (CurTok != '('){ // Simple variable ref.
    for (int i = 0; i < IdName.length(); i++){
      if (IdName.at(i) == '.'){
        std::string ObjectName =  IdName.substr(0, i-1);
        std::string MemberName =  IdName.substr(i+1, IdName.length()-1);
        return std::make_unique<ObjectMemberExprAST>(ObjectName, MemberName);
      }
    }
    int type;
    if (NamedValues[IdName] == nullptr){
      type = double_type;
    } else {
    int type = NamedValues[IdName]->type.type;
    }
    return std::make_unique<VariableExprAST>(IdName, type);
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
  // If this is a binop, find its precedence.
  while (true) {
    int TokPrec = GetTokPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec)
      return LHS;

    // Okay, we know this is a binop.
    int BinOp = CurTok;
    getNextToken(); // eat binop

    // Parse the primary expression after the binary operator.
    //auto RHS = ParsePrimary();
    auto RHS = ParseUnary();
    if (!RHS)
      return nullptr;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokPrecedence();
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
  auto LHS = ParseUnary();
  if (!LHS)
    return nullptr;
  return ParseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<ExprAST> ParsePrimary() {
  Log::Info() << "not operator " << CurTok << " " << IdentifierStr << "\n";
  switch (CurTok) {
  default:
    Log::Info() << "tok : " << CurTok << "\n";
    return LogError("Unknown token when expecting an expression");
  case tok_identifier:
    return ParseIdentifierExpr();
  case tok_number:
    return ParseNumberExpr();
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
  case tok_addr:
    return ParseAddrExpr();
  case tok_array_member:
    return ParseArrayMemberExpr();
  case tok_while:
    return ParseWhileExpr();
  }
}

std::unique_ptr<ExprAST> ParseArrayMemberExpr(){
  return std::make_unique<ArrayMemberExprAST>(strPosArray, posArrayNb);
}

std::unique_ptr<ExprAST> ParseTypeDeclaration(int* type, bool* is_ptr){
  Log::Info() << "type declaration found" << "\n";
  getNextToken(); // eat the ':'
  if (CurTok != tok_identifier)
    return LogError("expected identifier after var");
  if (is_type(IdentifierStr)){
    *type = get_type(IdentifierStr);
    Log::Info() << "Variable type : " << *type << "\n";
    getNextToken();
    if (CurTok == tok_ptr){
      *is_ptr = true;
      getNextToken();
    }
  } else {
    return LogError("wrong type found");
  }
  return nullptr;
}

static std::unique_ptr<PrototypeAST> ParsePrototype() {
  /*if (CurTok != tok_identifier)
    return LogErrorP("Expected function name in prototype");*/

  std::string FnName;
  unsigned Kind = 0;  // 0 = identifier, 1 = unary, 2 = binary.
  unsigned BinaryPrecedence = 30;
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

  if (CurTok != '(')
    return LogErrorP("Expected '(' in prototype");

  // Read the list of argument names.
  std::vector<std::pair<std::string,Cpoint_Type>> ArgNames;
  if (FnName == "main"){
    while (getNextToken() == tok_identifier);
    ArgNames.push_back(std::make_pair("argc", Cpoint_Type(double_type, false)));
    ArgNames.push_back(std::make_pair("argv",  Cpoint_Type(argv_type, false)));
  } else {
  getNextToken();
  while (CurTok == tok_identifier){
    std::string ArgName = IdentifierStr;
    getNextToken();
    int type = double_type;
    bool is_ptr = false;
    if (CurTok == ':'){
      ParseTypeDeclaration(&type, &is_ptr);
    }
    ArgNames.push_back(std::make_pair(ArgName, Cpoint_Type(type, is_ptr)));
  }
  }
  if (CurTok != ')')
    return LogErrorP("Expected ')' in prototype");

  // success.
  getNextToken();  // eat ')'.
  if (Kind && ArgNames.size() != Kind)
    return LogErrorP("Invalid number of operands for operator");
  Log::Info() << "Tok : " << CurTok << "\n";
  int type = -1;
  if (CurTok == tok_identifier){
    Log::Info() << "Tok type : " << CurTok << "\n";
    Log::Info() << "type : " << IdentifierStr << "\n";
    type = get_type(IdentifierStr);
    getNextToken(); // eat type
  }

  return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames), type, Kind != 0, BinaryPrecedence);
}

std::unique_ptr<ObjectDeclarAST> ParseObject(){
  isInObject = true;
  std::vector<std::unique_ptr<ExprAST>> VarList;
  getNextToken(); // eat object
  std::string objectName = IdentifierStr;
  Log::Info() << "Object Name : " << objectName << "\n";
  getNextToken();
  if (CurTok != '{')
    return LogErrorO("Expected '{' in prototype");
  getNextToken();
  while (CurTok == tok_var){
    auto declar = ParseVarExpr();
    if (!declar){return nullptr;}
    VarList.push_back(std::move(declar));
    //getNextToken();
    //std::cout << "currTok IN LOOP : " << CurTok << std::endl;
  }
  if (CurTok != '}'){
    return LogErrorO("Expected '}' in prototype");
  }
  getNextToken();  // eat '}'.
  isInObject = false;
  return std::make_unique<ObjectDeclarAST>(objectName, std::move(VarList));
}

std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken();  // eat func.
  auto Proto = ParsePrototype();
  if (!Proto) return nullptr;
  if (CurTok != '{'){
    LogErrorF("Expected '{' in function definition");
  }
  getNextToken();  // eat '{'
  std::vector<std::unique_ptr<ExprAST>> Body;
  while (CurTok != '}'){
    auto E = ParseExpression();
    if (!E){
       return nullptr;
    }
    Body.push_back(std::move(E));
  }
  getNextToken(); // eat }
  Log::Info() << "end of function" << "\n";
  return std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
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

std::unique_ptr<ExprAST> ParseAddrExpr(){
  getNextToken();  // eat addr.
  std::string Name = IdentifierStr;
  auto addr = std::make_unique<AddrExprAST>(Name);
  getNextToken();
  return addr;
}

std::unique_ptr<ExprAST> ParseUnary() {
  // If the current token is not an operator, it must be a primary expr.
  if (!isascii(CurTok) || CurTok == '(' || CurTok == ',' || CurTok == '{' || CurTok == ':' || CurTok == tok_string)
    return ParsePrimary();

  // If this is a unary operator, read it.
  Log::Info() << "Not an operator " << CurTok << "\n";
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
  auto Proto = std::make_unique<PrototypeAST>("main", std::vector<std::pair<std::string, Cpoint_Type>>());
  std::vector<std::unique_ptr<ExprAST>> Body;
  Body.push_back(std::move(E));
  return std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
}

std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken();  // eat the if.

  // condition.
  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;

  if (CurTok != tok_then)
    return LogError("expected then");
  getNextToken();  // eat the then

  auto Then = ParseExpression();
  if (!Then)
    return nullptr;
  Log::Info() << "CurTok : " << CurTok << "\n";
  if (CurTok != tok_else)
    return LogError("expected else");

  getNextToken();

  auto Else = ParseExpression();
  if (!Else)
    return nullptr;

  return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                      std::move(Else));
}


std::unique_ptr<ExprAST> ParseReturn(){
  getNextToken(); // eat the return
  Log::Info() << "CurTok : " << CurTok << " " << NumVal << "\n";

  auto return_value = ParseExpression();
  //auto Result = std::make_unique<ReturnAST>(NumVal);
  if (!return_value){
    return nullptr;
  }
  getNextToken();
  Log::Info() << "CurTok : " << CurTok << "\n";
  return std::make_unique<ReturnAST>(std::move(return_value));
  //return std::move(Result);
}

std::unique_ptr<ExprAST> ParseWhileExpr(){ 
  getNextToken();  // eat the while.
  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;
  Log::Info() << "CurTok : " << CurTok << "\n";
  if (CurTok != '{'){
    return LogError("expected {");
  }
  getNextToken();
  auto Body = ParseExpression();
  if (!Body)
    return nullptr;
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

  if (CurTok != tok_in)
    return LogError("expected 'in' after for");
  getNextToken();  // eat 'in'.

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

  return std::make_unique<ForExprAST>(IdName, std::move(Start),
                                       std::move(End), std::move(Step),
                                       std::move(Body));
}

std::unique_ptr<ExprAST> ParseVarExpr() {
  getNextToken(); // eat the var.

  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
  int type = -1;
  bool is_ptr = false;
  int nb_element = 0;
  bool is_array = false;
  // At least one variable name is required.
  if (CurTok != tok_identifier)
    return LogError("expected identifier after var");
  while (true) {
    std::string Name = IdentifierStr;
    getNextToken(); // eat identifier.
    // if declaration of array
    if (Name.find('[') != std::string::npos){
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
      for (int j = i; j < Name.size(); j++){
        if (Name.at(j) == ']'){
          break;
        } else {
          nb_element_str += Name.at(j);
        }
      }
      nb_element = std::stoi(nb_element_str);
      Name = Var_Name;
    }
    if (CurTok == ':'){
      auto a = ParseTypeDeclaration(&type, &is_ptr);
      if (a != nullptr){
        return a;
      }
    }
    // Read the optional initializer.
    std::unique_ptr<ExprAST> Init = nullptr;
    if (CurTok == '=') {
      getNextToken(); // eat the '='.

      Init = ParseExpression();
      if (!Init)
        return nullptr;
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
 std::unique_ptr<Cpoint_Type> cpoint_type = std::make_unique<Cpoint_Type>(type, is_ptr, is_array, nb_element);
  return std::make_unique<VarExprAST>(std::move(VarNames)/*, std::move(Body)*/, std::move(cpoint_type));
}
