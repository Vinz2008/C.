#include "llvm/IR/DIBuilder.h"
#include "debuginfo.h"
#include "errors.h"

using namespace llvm;


std::unique_ptr<DIBuilder> DBuilder;

extern std::unique_ptr<IRBuilder<>> Builder;;

extern struct DebugInfo CpointDebugInfo;

DISubroutineType *CreateFunctionType(Cpoint_Type type, std::vector<std::pair<std::string, Cpoint_Type>> Args) {
  SmallVector<Metadata *, 8> EltTys;
  DIType *DblTy = CpointDebugInfo.getDoubleTy(); // TODO : should create a function like get_llvm_type for debug info types

  // Add the result type.
  EltTys.push_back(DblTy);

  for (unsigned i = 0, e = Args.size(); i != e; ++i)
    EltTys.push_back(DblTy);

  return DBuilder->createSubroutineType(DBuilder->getOrCreateTypeArray(EltTys));
}

DIType *DebugInfo::getDoubleTy() {
  if (DblTy)
    return DblTy;

  DblTy = DBuilder->createBasicType("double", 64, dwarf::DW_ATE_float);
  return DblTy;
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
      DILocation::get(Scope->getContext(), context.line_nb, context.col_nb, Scope));
}