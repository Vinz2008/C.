#include <memory>
#include "codegen.h"
#include "ast.h"

extern int return_status;
extern Source_location emptyLoc;
extern int CurTok;

std::unique_ptr<ExprAST> vLogError(const char* Str, va_list args, Source_location astLoc){
  vlogErrorExit(std::make_unique<Compiler_context>(*Comp_context), Str, args, astLoc); // copy comp_context and not move it because it will be used multiple times
  return_status = 1;
  return nullptr;
}

std::unique_ptr<ExprAST> LogError(const char *Str, ...){
  va_list args;
  va_start(args, Str);
  //fprintf(stderr, "LogError: %s\n", Str);
  vLogError(Str, args, emptyLoc);
  va_end(args);
  return nullptr;
}


std::unique_ptr<PrototypeAST> LogErrorP(const char *Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, emptyLoc);
  va_end(args);
  return nullptr;
}

std::unique_ptr<ReturnAST> LogErrorR(const char *Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, emptyLoc);
  Log::Info() << "token : " << CurTok << "\n";
  va_end(args);
  return nullptr;
}


std::unique_ptr<StructDeclarAST> LogErrorS(const char *Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, emptyLoc);
  Log::Info() << "token : " << CurTok << "\n";
  va_end(args);
  return nullptr;
}

std::unique_ptr<FunctionAST> LogErrorF(const char *Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, emptyLoc);
  va_end(args);
  return nullptr;
}

std::unique_ptr<GlobalVariableAST> LogErrorG(const char *Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, emptyLoc);
  va_end(args);
  return nullptr;
}

std::unique_ptr<LoopExprAST> LogErrorL(const char* Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, emptyLoc);
  va_end(args);
  return nullptr;
}

std::unique_ptr<ModAST> LogErrorM(const char* Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, emptyLoc);
  va_end(args);
  return nullptr;
}

std::unique_ptr<TestAST> LogErrorT(const char* Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, emptyLoc);
  va_end(args);
  return nullptr;
}

std::unique_ptr<UnionDeclarAST> LogErrorU(const char* Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, emptyLoc);
  va_end(args);
  return nullptr;
}

std::unique_ptr<EnumDeclarAST> LogErrorE(const char* Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, emptyLoc);
  va_end(args);
  return nullptr;
}

Value* LogErrorV(Source_location astLoc, const char *Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, astLoc);
  va_end(args);
  return nullptr;
}

Function* LogErrorF(Source_location astLoc, const char *Str, ...){
  va_list args;
  va_start(args, Str);
  vLogError(Str, args, astLoc);
  va_end(args);
  return nullptr;
}