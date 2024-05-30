#include <iostream>
#include <unordered_map>
#include <fstream>
#include "lexer.h"
#include "preprocessor.h"
#include "errors.h"
#include "log.h"
#include "config.h"

using namespace std;

string IdentifierStr;
double NumVal;
string strStatic;
int charStatic;
extern std::unordered_map<std::string, int> BinopPrecedence;
string strPosArray;
int posArrayNb;
string OpStringMultiChar;
extern std::unique_ptr<Compiler_context> Comp_context;
extern Source_location emptyLoc;

int CurTok;

extern ifstream file_in;
extern ofstream file_log;

static int pos = 0;

string line = "<empty line>";
string last_line_str = "";

extern bool last_line;

void LogErrorLexer(const char *Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, emptyLoc);
  va_end(args);
}

void handlePreprocessor();

int getLine(std::istream &__is, std::string &__str){
  if (!getline(__is, __str)){
      last_line = true;
      file_log << "end of file" << "\n";
  }
  return 0;
}

std::string get_line_returned(){
  return line;
}


bool is_char_one_of_these(int c, std::string chars){
    for (int i = 0; i < chars.size(); i++){
        if (c == chars.at(i)){
            return true;
        }
    }
    return false;
}

void goToNextLine(std::istream &__is, std::string &__str){
  file_log << "new line" << "\n";
  Comp_context->lexloc.line_nb++;
  Log::Info() << "lines nb increment : " << Comp_context->lexloc.line_nb << " " << Comp_context->line << "\n";
  Comp_context->lexloc.col_nb = 0;
  Comp_context->lexloc.line = Comp_context->line;
  getLine(__is, __str);
  Comp_context->line = __str;
  file_log << "line size : " << __str.size() << "\n";
  pos = 0;
}

void go_to_next_line(){
  goToNextLine(file_in, line);
}

void init_line(){
  Log::Info() << "lines nb increment : " << Comp_context->lexloc.line_nb << " " << Comp_context->line << "\n";
  Comp_context->lexloc.col_nb = 0;
  getLine(file_in, line);
  Comp_context->line = line;
}

void handleEmptyLine();

void handlePreprocessor(){
  while (true){
    if (line.size() >= 2 && line.at(0) == '?' && line.at(1) == '['){  
      Log::Info() << "FOUND PREPROCESSOR INSTRUCTION" << "\n";
      preprocess_instruction(line);
      goToNextLine(file_in, line);
    } else {
      handleEmptyLine();
      preprocess_replace_variable(line);
      break;
    }
  }
}

void handleEmptyLine(){
  if (line.size() == 0){
    goToNextLine(file_in, line);
    handlePreprocessor();
  }
}

// is last char from stdin \n
bool is_last_char_next_line_jit = false;

int getCharLineStdin(){
    if (is_last_char_next_line_jit){
        return '\0';
    }
    int c = getchar();
    Log::Info() << "getCharLineStdin : " << c << "\n";
    if (c == '\n'){
        is_last_char_next_line_jit = true;
    }
    if (c == EOF){
        exit(0);
    }
    return c;
}

int peekCharLine(){
    if (pos <= line.size()){
        return line[pos];
    } else {
        return '\0';
    }
}

int getCharLine(){
#if ENABLE_JIT
  if (Comp_context->jit_mode){
    char c = getCharLineStdin();
    return c;
  }
#endif
  if (line == "<empty line>"){
    init_line();
    handlePreprocessor();
  }
  int c = line[pos];
  if (c == '\0'){
  while (c == '\0'){
    pos++;
    file_log << "\\0 found" << "\n";
    c = line[pos];
  }
  file_log << "next char after \\0 : " << line[pos] << "\n";
  }
  if (c == '\n' || c == '\r' || pos + 1 >= strlen(line.c_str())){
    goToNextLine(file_in, line);
    handlePreprocessor();
    //pos = 0;
    file_log << "next char in line : " << line[pos] << "\n";
  } else if (c == '\\'){
      switch (line[pos+1]){
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
     Comp_context->lexloc.col_nb++;
  }
  handleEmptyLine();
  file_log << "line : " << line << "\n";
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
  // Skip any whitespace.
  while (isspace(LastChar)){
    LastChar = getCharLine();
  }

  Comp_context->curloc = Comp_context->lexloc;
  if (isalpha(LastChar) || LastChar == '_') { // identifier: [a-zA-Z][a-zA-Z0-9]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getCharLine())) || LastChar == '_')
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
    if (IdentifierStr == "deref")
        return tok_deref;
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
    if (IdentifierStr == "match")
        return tok_match;
    if (IdentifierStr == "members")
        return tok_members;
    if (IdentifierStr == "type")
      return tok_typedef;
    if (IdentifierStr == "break")
      return tok_break;
    if (IdentifierStr == "typeId")
        return tok_typeid;
    if (IdentifierStr == "null")
        return tok_null;
    if (IdentifierStr == "cast")
        return tok_cast;
    if (IdentifierStr == "union")
      return tok_union;
    if (IdentifierStr == "enum")
        return tok_enum;
    if (IdentifierStr == "mod")
      return tok_mod;
    if (IdentifierStr == "true")
      return tok_true;
    if (IdentifierStr == "false")
      return tok_false;
    if (IdentifierStr == "defer")
        return tok_defer;
    if (IdentifierStr == "import"){
      go_to_next_line();
      return getNextToken();
    }
    Log::Info() << "lastChar : " << LastChar << "\n";
    Log::Info() << "IdentifierStr : " << IdentifierStr << "\n";
    return tok_identifier;
  }


  if (isdigit(LastChar)) { // Number: [0-9.]+
    std::string NumStr = "";
    if (LastChar == '0'){ // is hex (ex : 0x3F78)
        LastChar = getCharLine();
        if (LastChar == 'x' || LastChar == 'X'){
            LastChar = getCharLine();
            if (isdigit(LastChar) || is_char_one_of_these(LastChar, "abcdef") || is_char_one_of_these(LastChar, "ABCDEF")){
            do { // TODO : replace the two do whiles in this function with before them ifs by just whiles
                NumStr += LastChar;
                LastChar = getCharLine();
            } while (isdigit(LastChar) || is_char_one_of_these(LastChar, "abcdef") || is_char_one_of_these(LastChar, "ABCDEF"));
            }
            NumVal = strtol(NumStr.c_str(), nullptr, 16);
            return tok_number;
        } else {
            NumStr += '0';
            if (isdigit(LastChar) || LastChar == '.' || LastChar == '_'){
            if (LastChar != '_'){
                NumStr += LastChar;
            }
            LastChar = getCharLine();
            } else {
                goto after_number_lex_loop;
            }
        }
    }
    if (isdigit(LastChar) || LastChar == '.' || LastChar == '_'){
    do {
      if (LastChar != '_'){
          NumStr += LastChar;
      }
      LastChar = getCharLine();
      Log::Info() << "LastChar : " << LastChar << "\n";
      Log::Info() << "peek : " << peekCharLine() << "\n";
    } while (isdigit(LastChar) || (LastChar == '.' && isdigit(peekCharLine())) || LastChar == '_');
    }
after_number_lex_loop:
    Log::Info() << "NumStr : " << NumStr << "\n";
    NumVal = strtod(NumStr.c_str(), nullptr);
    Log::Info() << "NumVal : " << NumVal << "\n";
    return tok_number;
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
    LastChar = getCharLine();
    Log::Info() << "LastChar3 : " << LastChar << "\n";
    if (LastChar != '\''){
        LogErrorLexer("missing ' in char static declaration");
    }
    LastChar = getCharLine(); // TODO : have error if char is not '
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
  int FirstChar = LastChar;
  LastChar = getCharLine();
  // to debug multi-line operators
  // FirstChar : first character
  // LastChar : second character
  if (FirstChar == '/' && LastChar == '/'){
    return tok_single_line_comment;
  }
  if (LastChar == '|'){
    return create_multi_char_op(FirstChar, LastChar);
  }
  if (LastChar == '='){
    return create_multi_char_op(FirstChar, LastChar);
  }
  if (LastChar == '>'){
    return create_multi_char_op(FirstChar, LastChar);
  }
  if (LastChar == '<'){
    return create_multi_char_op(FirstChar, LastChar);
  }
  if (LastChar == '&'){
    return create_multi_char_op(FirstChar, LastChar);
  }
  return FirstChar;
}

int getNextToken() {
  CurTok = gettok();
  return CurTok;
}

int GetTokPrecedence() {
  Log::Info() << "CurTok in GetTokPrecedence : " << CurTok << "\n";
  if (!isascii(CurTok))
    return -1;

  // Make sure it's a declared binop.
  std::string op = "";
  op += CurTok;
  int TokPrec = BinopPrecedence[op];
  Log::Info() << "BinopPrecedence[\"" << op << "\"] : " << BinopPrecedence[op] << "\n";
  if (TokPrec <= 0) return -1;
  return TokPrec;
}

int getTokPrecedenceMultiChar(std::string op){
  int TokPrec = BinopPrecedence[op];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}
