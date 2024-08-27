#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Instructions.h"
//#include "ast.h"
#include "errors.h"
#include "types.h"

class ExprAST;

using namespace llvm;

#pragma once
struct DebugInfo {
  DICompileUnit* TheCU;
  // TODO : maybe replace all these by an unordered_map
  DIType* VoidTy;
  DIType* DoubleTy;
  DIType* FloatTy;
  /*DIType *IntTy;
  DIType *I8Ty;
  DIType *PtrTy;*/
  std::vector<DIScope*> LexicalBlocks;
  int pointer_width;
  DebugInfo(){}
  DebugInfo(int pointer_width) : TheCU(nullptr), VoidTy(nullptr), DoubleTy(nullptr), FloatTy(nullptr), LexicalBlocks(), pointer_width(pointer_width) {}
  DIType* getVoidTy();
  DIType* getDoubleTy();
  DIType* getFloatTy();
  DIType* getIntTy(int size);
  DIType* getPtrTy(DIType* baseDiType);
  void emitLocation(Compiler_context context, bool pop_the_scope);
  void emitLocation(ExprAST* AST);
};


DISubroutineType *DebugInfoCreateFunctionType(Cpoint_Type type, std::vector<std::pair<std::string, Cpoint_Type>> Args);
DICompositeType* DebugInfoCreateStructType(Cpoint_Type struct_type, std::vector<std::pair<std::string, Cpoint_Type>> Members, int LineNo);

void debugInfoCreateParameterVariable(DISubprogram *SP, DIFile *Unit, AllocaInst *Alloca, Cpoint_Type type, Argument& Arg, unsigned& ArgIdx, unsigned LineNo);

void debugInfoCreateLocalVariable(DIScope *SP, DIFile *Unit, AllocaInst *Alloca, Cpoint_Type type, unsigned LineNo);
void debugInfoCreateNamespace(std::string name);