#include "llvm/IR/DIBuilder.h"
#include "debuginfo.h"

using namespace llvm;


std::unique_ptr<DIBuilder> DBuilder;



DIType *DebugInfo::getDoubleTy() {
  if (DblTy)
    return DblTy;

  DblTy = DBuilder->createBasicType("double", 64, dwarf::DW_ATE_float);
  return DblTy;
}