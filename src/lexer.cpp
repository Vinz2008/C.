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
int charStatic;
extern std::map<std::string, int> BinopPrecedence;
string strPosArray;
int posArrayNb;
string OpStringMultiChar;
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

void gotToNextLine(std::istream &__is, std::string &__str){
  file_log << "new line" << "\n";
  Comp_context->line_nb++;
  Comp_context->col_nb = 0;
  getLine(__is, __str);
  Comp_context->line = __str;
  file_log << "line size : " << __str.size() << "\n";
  /*while (line.size == 0){
  goToNextLine(__is,__str);
  }*/
}

int getCharLine(){
  if (line == NULL){
    line = new string("");
    Comp_context->line_nb++;
    Comp_context->col_nb = 0;
    getLine(file_in, *line);
    Comp_context->line = *line;
if_preprocessor_first:
    Log::Info() << "Line : " << *line << "\n";
    if (line->at(0) == '?' && line->at(1) == '['){
      Log::Info() << "FOUND PREPROCESSOR INSTRUCTION" << "\n";
      preprocess_instruction(*line);
      getLine(file_in, *line);
      goto if_preprocessor_first;
    } else {
    //Log::Info() << "LAUNCH preprocess_replace_variable" << "\n";
    preprocess_replace_variable(*line);
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
    gotToNextLine(file_in, *line);
if_preprocessor:
    if (line->size() >= 2){
    if (line->at(0) == '?' && line->at(1) == '['){
      Log::Info() << "FOUND PREPROCESSOR INSTRUCTION" << "\n";
      preprocess_instruction(*line);
      gotToNextLine(file_in, *line);
      goto if_preprocessor;
    } else {
      //Log::Info() << "LAUNCH preprocess_replace_variable" << "\n";
      preprocess_replace_variable(*line);
    }
    }
    /*if (!getline(file_in, *line)){
      last_line = true;
    }*/
    pos = 0;
    file_log << "next char in line : " << (*line)[pos] << "\n";
  } else if (c == '\\'){
      switch ((*line)[pos+1]){
        default:
          break;
        case 'n':
          c = '\n';
          pos+=2;
          break;
        case 't':
          c = '\t';
          pos+=2;
          break;
        case '0':
          c = '\0';
          pos+=2;
          break;
      }
  } else {
     pos++;
     Comp_context->col_nb++;
  }
  /*if (line->size() == 0 && (*line)[pos] == '\0'){
    file_log << "empty line" << "\n";
    gotToNextLine(file_in, *line);
  }*/
  file_log << "line : " << *line << "\n";
  file_log << "c : " << (char)c << "\n";
  file_log << "c int : " << c << "\n";
  return c;
}

static int create_multi_char_op(int FirsChar, int SecondChar){
  OpStringMultiChar = FirsChar;
  OpStringMultiChar += SecondChar;
  return tok_op_multi_char;
}

static int gettok() {
  static int LastChar = ' ';
  //std::cout << "start gettok" << std::endl;
  // Skip any whitespace.
  while (isspace(LastChar)){
    //cout << "SPACE" << endl;
    LastChar = getCharLine();
  }
  if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getCharLine())) || /*LastChar == '[' || LastChar == ']' || LastChar == '.' ||*/ LastChar == '_')
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
    if (IdentifierStr == "struct")
      return tok_struct;
    if (IdentifierStr == "class")
      return tok_class;
    if (IdentifierStr == "addr")
      return tok_addr;
    if (IdentifierStr == "sizeof")
      return tok_sizeof;
    if (IdentifierStr == "ptr")
      return tok_ptr;
    if (IdentifierStr == "while")
      return tok_while;
    if (IdentifierStr == "loop")
      return tok_loop;
    if (IdentifierStr == "goto")
      return tok_goto;
    if (IdentifierStr == "label")
      return tok_label;
    if (IdentifierStr == "true")
      return tok_true;
    if (IdentifierStr == "false")
      return tok_false;
    if (IdentifierStr == "import"){
      go_to_next_line();
      return getNextToken();
    }
    Log::Info() << "lastChar : " << LastChar << "\n";
    Log::Info() << "IdentifierStr : " << IdentifierStr << "\n";
    return tok_identifier;
  }


  if (isdigit(LastChar)) { // Number: [0-9.]+
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
    strStatic = LastChar;
    while ((LastChar = getCharLine()) != '\"') {
      strStatic += LastChar;
    }
    LastChar = getCharLine();
    Log::Info() << "strStatic : " << strStatic << "\n";
    Log::Info() << "LastChar : " << (char)LastChar << "\n";
    return tok_string;
  }

  if (LastChar == '\''){
    Log::Info() << "LastChar : " << LastChar << "\n";
    LastChar = getCharLine();
    charStatic = LastChar;
    Log::Info() << "LastChar2 : " << LastChar << "\n";
    /*charStatic = LastChar;
    Log::Info() << "charStatic : " << LastChar << "\n";
    LastChar = getCharLine();
    Log::Info() << "LastChar : " << LastChar << "\n";
    if (LastChar != '\''){
      LogError("Missing '");
    }*/
    LastChar = getCharLine();
    LastChar = getCharLine();
    return tok_char;
  }
  if (LastChar == '.'){
    std::string format = "";
    do {
      format += LastChar;
    } while ((LastChar = getCharLine()) == '.');
    if (format == "..."){
      return tok_format;
    } else if (format == "."){
      return '.';
    } else {
      Log::Info() << "format : " << format << "\n";
      LogError("error in format\n");
    }
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF)
    return tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  LastChar = getCharLine();
  // to debug multi-line operators
  //Log::Info() << "LastChar : " << LastChar << " " << "ThisChar : " << ThisChar << "\n";
  // ThisChar : first character
  // LastChar : second character
  if (LastChar == '|'){
    return create_multi_char_op(ThisChar, LastChar);
  }
  if (LastChar == '='){
    return create_multi_char_op(ThisChar, LastChar);
  }
  if (LastChar == '>'){
    return create_multi_char_op(ThisChar, LastChar);
  }
  if (LastChar == '<'){
    return create_multi_char_op(ThisChar, LastChar);
  }
  if (LastChar == '&'){
    return create_multi_char_op(ThisChar, LastChar);
  }
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
  std::string op = "";
  op += CurTok;
  int TokPrec = BinopPrecedence[op];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}

int getTokPrecedenceMultiChar(std::string op){
  int TokPrec = BinopPrecedence[op];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}
