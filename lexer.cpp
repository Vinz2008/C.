#include <iostream>
#include <map>
#include <fstream>
#include "lexer.h"
#include "preprocessor.h"
#include "errors.h"
#include "log.h"

using namespace std;

string IdentifierStr;
double NumVal;
string strStatic;
extern std::map<char, int> BinopPrecedence;
string strPosArray;
int posArrayNb;
extern std::unique_ptr<Compiler_context> Comp_context;

int CurTok;

extern ifstream file_in;
extern ofstream file_log;

static int pos = 0;

string* line = NULL;
string last_line_str = "";

extern bool last_line;

int getLine(std::istream &__is, std::string &__str){
  if (!getline(__is, __str)){
      last_line = true;
      file_log << "end of file" << "\n";
  }
  //Comp_context->line_nb++;
  /*Comp_context->line = last_line_str;
  last_line_str = __str*/;
  //Comp_context->line = __str;
  return 0;
}

std::string get_line_returned(){
  return *line;
}

void go_to_next_line(){
  getLine(file_in, *line);
}

int getCharLine(){
  if (line == NULL){
    line = new string("");
    Comp_context->line_nb++;
    Comp_context->col_nb = 0;
    getLine(file_in, *line);
    Comp_context->line = *line;
if_preprocessor_first:
    if (line->at(0) == '?' && line->at(1) == '['){
      Log::Info() << "FOUND PREPROCESSOR INSTRUCTION" << "\n";
      preprocess_instruction(*line);
      getLine(file_in, *line);
      goto if_preprocessor_first;
    }
    /*if (!getline(file_in, *line)){
      last_line = true;
    }*/
  }
  int c = (*line)[pos];
  if (c == '\0'){
start_backslash_zero:
    pos++;
    file_log << "\\0 found" << "\n";
    c = (*line)[pos];
    if (c == '\0'){
      goto start_backslash_zero;
    }
    file_log << "next char after \\0 : " << (*line)[pos] << "\n";
  }
  if (c == '\n' || c == '\r' /*|| c == '\0'*/ || pos + 1 >= strlen(line->c_str())){
    file_log << "new line" << "\n";
    Comp_context->line_nb++;
    Comp_context->col_nb = 0;
    getLine(file_in, *line);
    Comp_context->line = *line;
if_preprocessor:
    if (line->size() >= 2){
    if (line->at(0) == '?' && line->at(1) == '['){
      Log::Info() << "FOUND PREPROCESSOR INSTRUCTION" << "\n";
      getLine(file_in, *line);
      goto if_preprocessor;
    }
    }
    /*if (!getline(file_in, *line)){
      last_line = true;
    }*/
    pos = 0;
    file_log << "next char in line : " << (*line)[pos] << "\n";
  } else {
     pos++;
     Comp_context->col_nb++;
  }
  file_log << "line : " << *line << "\n";
  file_log << "c : " << (char)c << "\n";
  file_log << "c int : " << c << "\n";
  return c;
}

static int gettok() {
  static int LastChar = ' ';
  //std::cout << "start gettok" << std::endl;
  // Skip any whitespace.
  while (isspace(LastChar)){
    //cout << "SPACE" << endl;
    LastChar = getCharLine();
  }

  if (isalpha(LastChar) || LastChar == '[' || LastChar == ']') { // identifier: [a-zA-Z][a-zA-Z0-9]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getCharLine())) || LastChar == '[' || LastChar == ']')
      IdentifierStr += LastChar;

    if (IdentifierStr == "func")
      return tok_func;
    if (IdentifierStr == "extern")
      return tok_extern;
    if (IdentifierStr == "if")
      return tok_if;
    if (IdentifierStr == "then")
      return tok_then;
    if (IdentifierStr == "else")
      return tok_else;
    if (IdentifierStr == "for")
      return tok_for;
    if (IdentifierStr == "in")
      return tok_in;
    if (IdentifierStr == "return")
      return tok_return;
    if (IdentifierStr == "binary")
      return tok_binary;
    if (IdentifierStr == "unary")
      return tok_unary;
    if (IdentifierStr == "var")
      return tok_var;
    if (IdentifierStr == "object")
      return tok_object;
    if (IdentifierStr == "addr")
      return tok_addr;
    if (IdentifierStr == "ptr")
      return tok_ptr;
    if (IdentifierStr == "while")
      return tok_while;
    Log::Info() << "lastChar : " << LastChar << "\n";
    /*if (LastChar == '['){
      do {
        strPosArray += LastChar;
        LastChar = getCharLine();
      } while (isdigit(LastChar));
      posArrayNb = strtod(strPosArray.c_str(), nullptr);
      return tok_array_member;
    }*/
    Log::Info() << "IdentifierStr : " << IdentifierStr << "\n";
    return tok_identifier;
  }


  if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9.]+
    std::string NumStr;
    do {
      if (LastChar != '_'){
          NumStr += LastChar;
      }
      LastChar = getCharLine();
    } while (isdigit(LastChar) || LastChar == '.' || LastChar == '_');

    NumVal = strtod(NumStr.c_str(), nullptr);
    return tok_number;
  }

  if (LastChar == '#') {
    // Comment until end of line.
    do
      LastChar = getCharLine();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
  }
  if (LastChar == '\"'){
    LastChar = getCharLine();
    do {
      strStatic += LastChar;
      LastChar = getCharLine();
    } while (LastChar != '\"');
    LastChar = getCharLine();
    Log::Info() << "strStatic : " << strStatic << "\n";
    Log::Info() << "LastChar : " << (char)LastChar << "\n";
    return tok_string;
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF)
    return tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  LastChar = getCharLine();
  return ThisChar;
}

int getNextToken() {
  //std::cout << "Before CurTok" << std::endl;
  CurTok = gettok();
  //cout << "CurTok : " << CurTok << endl;
  return CurTok;
}

int GetTokPrecedence() {
  if (!isascii(CurTok))
    return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}
