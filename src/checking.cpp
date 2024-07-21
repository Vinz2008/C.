#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "codegen.h"
#include "types.h"
#include "operators.h"

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<IRBuilder<>> Builder;

extern Source_location emptyLoc;

void add_manually_extern(std::string fnName, Cpoint_Type cpoint_type, std::vector<std::pair<std::string, Cpoint_Type>> ArgNames, unsigned Kind, unsigned BinaryPrecedence, bool is_variable_number_args, bool has_template, std::string TemplateName);

Value* bound_checking_constant_index_array_member(Constant* indexConst, Cpoint_Type cpoint_type, Source_location loc){
    double indexd = -INFINITY;
    if (indexConst->getType() == get_type_llvm(double_type) || indexConst->getType() == get_type_llvm(float_type)){
      ConstantFP* indexConstFP = dyn_cast<ConstantFP>(indexConst);
      indexd = indexConstFP->getValue().convertToDouble();
    } else if (indexConst->getType() == get_type_llvm(i32_type)){
      ConstantInt* IndexConstInt = dyn_cast<ConstantInt>(indexConst);
      indexd = IndexConstInt->getValue().roundToDouble();
    }
    if (indexd != -INFINITY){
      if (indexd > cpoint_type.nb_element){
        return LogErrorV(loc, "Index too big for array");
      }
    }
    return ConstantFP::get(*TheContext, APFloat((double)0)); // to differenciate from nullptr (TODO : fix this ?)
}

Value* bound_checking_dynamic_index_array_member(Value* index, Cpoint_Type cpoint_type){
    std::vector<std::pair<std::string, Cpoint_Type>> PanicArgs;
    PanicArgs.push_back(std::make_pair("message", Cpoint_Type(i8_type, true)));
    add_manually_extern("panic", Cpoint_Type(void_type), std::move(PanicArgs), 0, 30, false, false, "");
    //Value* nbElement = ConstantFP::get(*TheContext, APFloat((double) cpoint_type.nb_element));
    Value* nbElement = ConstantInt::get(get_type_llvm(Cpoint_Type(i32_type)), APInt(32, (uint64_t)cpoint_type.nb_element, false));
    if (index->getType() != nbElement->getType()){
      convert_to_type(/*get_cpoint_type_from_llvm(nbElement->getType())*/ Cpoint_Type(i32_type), index->getType(), nbElement);
    }
    Value* CondV = operators::LLVMCreateGreaterOrEqualThan(index, nbElement, get_cpoint_type_from_llvm(index->getType()));
    if (!CondV)
      return nullptr;
    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "bound_checking_then", TheFunction);
    BasicBlock *AfterBB = BasicBlock::Create(*TheContext, "bound_checking_after", TheFunction);
    Builder->CreateCondBr(CondV, ThenBB, AfterBB);
    Builder->SetInsertPoint(ThenBB);
    std::vector<std::unique_ptr<ExprAST>> Args;
    Args.push_back(std::make_unique<StringExprAST>("Out of bound access of array"));
    std::make_unique<CallExprAST>(emptyLoc, "panic", std::move(Args), Cpoint_Type(double_type))->codegen();
    Builder->CreateBr(AfterBB);
    Builder->SetInsertPoint(AfterBB);
    return ConstantFP::get(*TheContext, APFloat((double)0)); // to differenciate from nullptr
}