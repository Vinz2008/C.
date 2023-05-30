#include "llvm/IR/DIBuilder.h"
#include "debuginfo.h"
#include "errors.h"

using namespace llvm;


std::unique_ptr<DIBuilder> DBuilder;

extern std::unique_ptr<IRBuilder<>> Builder;;

extern struct DebugInfo CpointDebugInfo;

extern bool debug_info_mode;

DIType *DebugInfo::getDoubleTy() {
  if (DblTy)
    return DblTy;
  DblTy = DBuilder->createBasicType("double", 64, dwarf::DW_ATE_float);
  return DblTy;
}

DIType* get_debuginfo_type(Cpoint_Type type){
  switch (type.type){  
  default:
  case double_type:
    return CpointDebugInfo.getDoubleTy();
  }
}

DISubroutineType *DebugInfoCreateFunctionType(Cpoint_Type type, std::vector<std::pair<std::string, Cpoint_Type>> Args) {
  SmallVector<Metadata *, 8> EltTys;
  DIType *DblTy = get_debuginfo_type(type);

  // Add the result type.
  EltTys.push_back(DblTy);

  for (unsigned i = 0, e = Args.size(); i != e; ++i)
    EltTys.push_back(DblTy);

  return DBuilder->createSubroutineType(DBuilder->getOrCreateTypeArray(EltTys));
}


// Add to each class a location attribute for line and column position to create debug infos like in the Kaleidoscope tutorial 

void DebugInfo::emitLocation(Compiler_context context, bool pop_the_scope = false){
  if (pop_the_scope){
    return Builder->SetCurrentDebugLocation(DebugLoc());
  }
  DIScope *Scope;
  if (LexicalBlocks.empty())
    Scope = TheCU;
  else
    Scope = LexicalBlocks.back();
  Builder->SetCurrentDebugLocation(
      DILocation::get(Scope->getContext(), context.loc.line_nb, context.loc.col_nb, Scope));
}

void DebugInfo::emitLocation(ExprAST* AST){
  if (debug_info_mode){
  if (!AST){
    return Builder->SetCurrentDebugLocation(DebugLoc());
  }
  DIScope *Scope;
  if (LexicalBlocks.empty())
    Scope = TheCU;
  else
    Scope = LexicalBlocks.back();
  Builder->SetCurrentDebugLocation(
      DILocation::get(Scope->getContext(), AST->getLine(), AST->getCol(), Scope));
  }
}

// will probably not use it because it is used not often and we need most of these values
void debugInfoCreateFunction(PrototypeAST &P, Function *TheFunction){
  DIFile *Unit = DBuilder->createFile(CpointDebugInfo.TheCU->getFilename(),
                                      CpointDebugInfo.TheCU->getDirectory());
  DIScope *FContext = Unit;
  unsigned ScopeLine = 0; // TODO : add way to get file line from Prototype
  unsigned LineNo = 0;
  DISubprogram *SP = DBuilder->createFunction(
    FContext, P.getName(), StringRef(), Unit, LineNo,
    DebugInfoCreateFunctionType(P.cpoint_type, P.Args),
    ScopeLine,
    DINode::FlagPrototyped,
    DISubprogram::SPFlagDefinition);
  TheFunction->setSubprogram(SP);
}



void debugInfoCreateParameterVariable(DISubprogram *SP, DIFile *Unit, AllocaInst *Alloca, Argument& Arg, unsigned& ArgIdx, unsigned LineNo){
  if (debug_info_mode){
  DILocalVariable *D = DBuilder->createParameterVariable(
      SP, Arg.getName(), ++ArgIdx, Unit, LineNo, CpointDebugInfo.getDoubleTy(),
      true);

  DBuilder->insertDeclare(Alloca, D, DBuilder->createExpression(),
                        DILocation::get(SP->getContext(), LineNo, 0, SP),
                        Builder->GetInsertBlock());
  }
}