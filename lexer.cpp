#include <iostream>
#include <map>
#include <fstream>
#include "lexer.h"

using namespace std;

string IdentifierStr;
double NumVal;
extern std::map<char, int> BinopPrecedence;

int CurTok;

extern ifstream file_in;
extern ofstream file_log;

static int pos = 0;

string* line = NULL;

extern bool last_line;

int getCharLine(){
  if (line == NULL){
    line = new string("");
    if (!getline(file_in, *line)){
      last_line = true;
    }
  }
  int c = (*line)[pos];
  if (c == '\n' || c == '\r' || c == '\0' || pos + 1 >= strlen(line->c_str())){
    if (!getline(file_in, *line)){
      last_line = true;
    }
    pos = 0;
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

    if (IdentifierStr == "def")
      return tok_def;
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

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF)
    return tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  LastChar = getCharLine();
  return ThisChar;
}

int getNextToken() {
  return CurTok = gettok();
}

int GetTokPrecedence() {
  if (!isascii(CurTok))
    return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}