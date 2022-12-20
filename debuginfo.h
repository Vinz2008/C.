#include "llvm/IR/DIBuilder.h"
#include "ast.h"

using namespace llvm;

#ifndef _DEBUGINFO_HEADER_
#define _DEBUGINFO_HEADER_
struct DebugInfo {
  DICompileUnit *TheCU;
  DIType *DblTy;
  std::vector<DIScope *> LexicalBlocks;
  DIType *getDoubleTy();
};


#endif
