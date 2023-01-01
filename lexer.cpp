#include <iostream>
#include <map>
#include <fstream>
#include "lexer.h"

using namespace std;

string IdentifierStr;
double NumVal;
string strStatic;
extern std::map<char, int> BinopPrecedence;

int CurTok;

extern ifstream file_in;
extern ofstream file_log;

static int pos = 0;

string* line = NULL;

extern bool last_line;

int getLine(std::istream &__is, std::string &__str){
  if (!getline(__is, __str)){
      last_line = true;
      file_log << "end of file" << "\n";
      
  }
  return 0;
}

int getCharLine(){
  if (line == NULL){
    line = new string("");
    getLine(file_in, *line);
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
    getLine(file_in, *line);
    /*if (!getline(file_in, *line)){
      last_line = true;
    }*/
    pos = 0;
    file_log << "next char in line : " << (*line)[pos] << "\n";
  } else {
     pos++;
  }
  file_log << "line : " << *line << "\n";
  file_log << "c : " << (char)c << "\n";
  file_log << "c int : " << c << "\n";
  return c;
}

static int gettok() {
  static int LastChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar))
    LastChar = getCharLine();

  if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getCharLine())))
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
    cout << "IdentifierStr : " << IdentifierStr << endl;
    return tok_identifier;
  }

  if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9.]+
    std::string NumStr;
    do {
      NumStr += LastChar;
      LastChar = getCharLine();
    } while (isdigit(LastChar) || LastChar == '.');

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
    cout << "strStatic : " << strStatic << endl;
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
