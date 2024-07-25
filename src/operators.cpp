#include "operators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DIBuilder.h"
#include "types.h"
#include "ast.h"
#include "codegen.h"

using namespace llvm;
extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unordered_map<std::string, int> BinopPrecedence;

namespace operators {

Value* LLVMCreateAdd(Value* L, Value* R, Cpoint_Type arg_type){
    /*if (arg_type.is_vector_type){
        Cpoint_Type vector_element_type = *arg_type.vector_element_type;
        if (vector_element_type.is_signed() || vector_element_type.is_unsigned()){
            std::vector<llvm::Value *> ArgsV = { L, R};
            std::vector<Type*> Tys = { get_type_llvm(arg_type) };
            std::vector<Constant*> vector_mask_constants;
            Constant* true_constant = ConstantInt::get(get_type_llvm(bool_type), APInt(1, (uint64_t)1, false));
            for (int i = 0; i < arg_type.vector_size; i++){
                vector_mask_constants.push_back(true_constant);
            }
            auto vector_mask = ConstantVector::get(vector_mask_constants);
            ArgsV.push_back(vector_mask);
            auto constant_vector_length = ConstantInt::get(get_type_llvm(i32_type), APInt(32, (uint64_t)arg_type.vector_size, false));
            ArgsV.push_back(constant_vector_length);
            return callLLVMIntrisic("llvm.vp.add", ArgsV, Tys);
        }
    }*/
    if (!get_cpoint_type_from_llvm(R->getType()).is_decimal_number_type() || arg_type.is_vector_type){
      return Builder->CreateAdd(L, R, "addtmp");
    }
    return Builder->CreateFAdd(L, R, "faddtmp");
}

Value* LLVMCreateSub(Value* L, Value* R, Cpoint_Type arg_type){
    if (!get_cpoint_type_from_llvm(R->getType()).is_decimal_number_type() || arg_type.is_vector_type){
      return Builder->CreateSub(L, R, "subtmp");
    }
    return Builder->CreateFSub(L, R, "fsubtmp");
}

Value* LLVMCreateMul(Value* L, Value* R, Cpoint_Type arg_type){
    if (!get_cpoint_type_from_llvm(R->getType()).is_decimal_number_type() || arg_type.is_vector_type){
      return Builder->CreateMul(L, R, "multmp");
    }
    return Builder->CreateFMul(L, R, "fmultmp");
}

Value* LLVMCreateDiv(Value* L, Value* R, Cpoint_Type arg_type){
    //Cpoint_Type type = get_cpoint_type_from_llvm(R->getType());
    if (arg_type.is_vector_type){
        arg_type = *arg_type.vector_element_type;
    }
    if (!arg_type.is_decimal_number_type()){
      if (arg_type.is_signed()){
        return Builder->CreateSDiv(L, R, "sdivtmp");
      } else {
        return Builder->CreateUDiv(L, R, "udivtmp");
      }
    }
    return Builder->CreateFDiv(L, R, "fdivtmp");
}

Value* LLVMCreateRem(Value* L, Value* R/*, Cpoint_Type type*/){ // TODO : uncomment
    Cpoint_Type type = get_cpoint_type_from_llvm(R->getType());
    if (!type.is_decimal_number_type()){
      if (type.is_signed()){
        return Builder->CreateSRem(L, R, "sremtmp");
      } else {
        return Builder->CreateURem(L, R, "uremtmp");
      }
    }
    return Builder->CreateFRem(L, R, "fremtmp");
}

Value* LLVMCreateCmp(Value* L, Value* R, Cpoint_Type arg_type){
    if (arg_type.is_vector_type){
        arg_type = *arg_type.vector_element_type;
    }
    if (!arg_type.is_decimal_number_type()){
      L = Builder->CreateICmpEQ(L, R, "cmptmp");
    } else {
      L = Builder->CreateFCmpUEQ(L, R, "cmptmp");
    }
    return L;
}

Value* LLVMCreateNotEqualCmp(Value* L, Value* R, Cpoint_Type arg_type){
    if (arg_type.is_vector_type){
        arg_type = *arg_type.vector_element_type;
    }
    if (!arg_type.is_decimal_number_type()){
        L = Builder->CreateICmpNE(L, R, "notequalcmptmp");
    } else {
        L = Builder->CreateFCmpUNE(L, R, "notequalfcmptmp");
    }
    return L;
}

Value* LLVMCreateGreaterThan(Value* L, Value* R, Cpoint_Type arg_type){
    if (arg_type.is_vector_type){
        arg_type = *arg_type.vector_element_type;
    }
    if (!arg_type.is_decimal_number_type()){
      L = Builder->CreateICmpSGT(L, R, "cmptmp");
    } else {
      L = Builder->CreateFCmpOGT(L, R, "cmptmp");
    }
    return L;
}

Value* LLVMCreateGreaterOrEqualThan(Value* L, Value* R, Cpoint_Type arg_type){
    if (arg_type.is_vector_type){
        arg_type = *arg_type.vector_element_type;
    }
    if (!arg_type.is_decimal_number_type()){
        L = Builder->CreateICmpSGE(L, R, "cmptmp");
    } else {
        L = Builder->CreateFCmpOGE(L, R, "cmptmp");
    }
    return L;
}

Value* LLVMCreateSmallerThan(Value* L, Value* R, Cpoint_Type arg_type){
    if (arg_type.is_vector_type){
        arg_type = *arg_type.vector_element_type;
    }
    if (!arg_type.is_decimal_number_type()){
      L = Builder->CreateICmpSLT(L, R, "cmptmp");
    } else {
      L = Builder->CreateFCmpOLT(L, R, "cmptmp");
    }
    return L;
}

Value* LLVMCreateSmallerOrEqualThan(Value* L, Value* R, Cpoint_Type arg_type){
    if (arg_type.is_vector_type){
        arg_type = *arg_type.vector_element_type;
    }
    if (!arg_type.is_decimal_number_type()){
        L = Builder->CreateICmpSLE(L, R, "cmptmp");
    } else {
        L = Builder->CreateFCmpOLE(L, R, "cmptmp");
    }
    return L;
}

Value* LLVMCreateLogicalOr(Value* L, Value* R){
    return Builder->CreateOr(L, R, "logicalortmp");
}

Value* LLVMCreateLogicalAnd(Value* L, Value* R){
    return Builder->CreateAnd(L, R, "logicalandtmp");
}

Value* LLVMCreateAnd(Value* L, Value* R){
    return Builder->CreateAnd(L, R, "andtmp");
}

void installPrecendenceOperators(){
    // Install standard binary operators.
    // 1 is lowest precedence.
    BinopPrecedence["="] = 5;
    BinopPrecedence["||"] = 10;
    BinopPrecedence["&&"] = 11;
    BinopPrecedence["|"] = 12;
    BinopPrecedence["^"] = 13;
    BinopPrecedence["&"] = 14;

    BinopPrecedence["!="] = 15;
    BinopPrecedence["=="] = 15;

    BinopPrecedence["<"] = 16;
    BinopPrecedence["<="] = 16;
    BinopPrecedence[">"] = 16;
    BinopPrecedence[">="] = 16;

    BinopPrecedence["<<"] = 20;
    BinopPrecedence[">>"] = 20;

    BinopPrecedence["+"] = 25;
    BinopPrecedence["-"] = 25;

    BinopPrecedence["*"] = 30;
    BinopPrecedence["%"] = 30;
    BinopPrecedence["/"] = 30;

    BinopPrecedence["["] = 35;

    BinopPrecedence["."] = 35;

    BinopPrecedence["("] = 35;
    BinopPrecedence["~"] = 35;
}

}

Cpoint_Type UnaryExprAST::get_type(){
    Cpoint_Type operand_type = Operand->get_type();
    if (Opcode == '-'){
        return operand_type;
    }
    if (Opcode == '&'){
        Cpoint_Type new_type = operand_type;
        new_type.is_ptr = true;
        new_type.nb_ptr++;
    }
    if (Opcode == '*'){
        return operand_type.deref_type();
    }
    return Cpoint_Type(); // TODO : add all operators
}

#include <typeinfo>

Cpoint_Type BinaryExprAST::get_type(){
    if (Op == "=" || Op == "<<" || Op == ">>" || Op == "|" || Op == "^" || Op == "&" || Op == "+" || Op == "-" || Op == "*" || Op == "%" || Op == "/"){
        return LHS->get_type();
    }
    if (Op == "||" || Op == "&&" || Op == "==" || Op == "!=" || Op == "<" || Op == "<=" || Op == ">" || Op == ">="){
        Cpoint_Type LHS_type = LHS->get_type();
        if (LHS_type.is_vector_type && Op == "=="){
            return Cpoint_Type(other_type, false, 0, false, 0, false, "", false, "", false, "", false, false, nullptr, false, {}, nullptr, true, new Cpoint_Type(bool_type), LHS_type.vector_size);
        }
        return Cpoint_Type(bool_type);
    }
    if (Op == "["){
        return LHS->get_type().deref_type();
    }
    if (Op == "."){
        auto varExprMember = get_Expr_from_ExprAST<VariableExprAST>(RHS->clone());
        if (!varExprMember){
            LogError("expected an identifier for a struct member");
            return Cpoint_Type();
        }
        std::string MemberName = varExprMember->Name;
        //Log::Info() << "LHS name struct member : " << typeid(*LHS.get()).name() << "\n";
        Cpoint_Type LHS_type = LHS->get_type();
        return get_member_type_and_pos_object(LHS_type, MemberName)->first;
    }
    // TODO : add all operators
    LogError("trying to get the type from an operator for which the function it is not implemented");
    return Cpoint_Type();
}