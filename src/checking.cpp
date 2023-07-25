#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "codegen.h"
#include "types.h"

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;

Value* bound_checking_constant_index_array_member(Constant* indexConst, Cpoint_Type cpoint_type, Source_location loc){
    double indexd = -INFINITY;
    if (indexConst->getType() == get_type_llvm(double_type)){
      ConstantFP* indexConstFP = dyn_cast<ConstantFP>(indexConst);
      indexd = indexConstFP->getValue().convertToDouble();
    } else if (indexConst->getType() == get_type_llvm(int_type)){
      ConstantInt* IndexConstInt = dyn_cast<ConstantInt>(indexConst);
      indexd = IndexConstInt->getValue().roundToDouble();
    }
    if (indexd != -INFINITY){
      if (indexd > cpoint_type.nb_element){
        return LogErrorV(loc, "Index too big for array");
      }
    }
    return ConstantFP::get(*TheContext, APFloat((double)0)); // to differenciate from nullptr
}