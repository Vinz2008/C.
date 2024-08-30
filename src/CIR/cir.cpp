//#include "config.h"
#include "cir.h"

#if ENABLE_CIR

#include "../ast.h"

// TODO verify that last instruction of function is never type (a return, an unreachable, etc)

CIR::InstructionRef VariableExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return fileCIR->CurrentFunction->vars[Name];
}

CIR::InstructionRef DeferExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef ReturnAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto return_inst = std::make_unique<CIR::ReturnInstruction>(returned_expr->cir_gen(fileCIR));
    return fileCIR->add_instruction(std::move(return_inst));
}

CIR::InstructionRef ScopeExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef AsmExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}


CIR::InstructionRef AddrExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef VarExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::InstructionRef varInit;
    if (VarNames.at(0).second){
        varInit = VarNames.at(0).second->cir_gen(fileCIR);
    }
    auto var_ref = fileCIR->add_instruction(std::make_unique<CIR::VarInit>(std::make_pair(VarNames.at(0).first, varInit)));
    fileCIR->CurrentFunction->vars[VarNames.at(0).first] = var_ref;
    return CIR::InstructionRef();
}

CIR::InstructionRef EmptyExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef StringExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef DerefExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef IfExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef TypeidExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef CharExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::ConstNumber::nb_val_ty nb_val;
    nb_val.int_nb = c;
    return fileCIR->add_instruction(std::make_unique<CIR::ConstNumber>(false, Cpoint_Type(i8_type), nb_val));
    //return CIR::InstructionRef();
}

CIR::InstructionRef SemicolonExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef SizeofExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef BoolExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef LoopExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef ForExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef StructMemberCallExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef MatchExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef ConstantVectorExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef EnumCreation::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef CallExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    // for debug for new
    auto call_instr = std::make_unique<CIR::CallInstruction>(Callee, std::vector<CIR::InstructionRef>());
    for (int i = 0; i < Args.size(); i++){
        call_instr->Args.push_back(Args.at(i)->cir_gen(fileCIR));
    }
    return fileCIR->add_instruction(std::move(call_instr));
}

CIR::InstructionRef GotoExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef WhileExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef BreakExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef ConstantStructExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef BinaryExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef ConstantArrayExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef LabelExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef NumberExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    Cpoint_Type number_type = this->get_type();
    union CIR::ConstNumber::nb_val_ty nb_val;
    bool is_float = number_type.type == double_type;
    if (is_float){ 
        nb_val.float_nb = Val; 
    } else { 
        nb_val.int_nb = (int)Val; 
    }
    // insert in insertpoint instead
    return fileCIR->add_instruction(std::make_unique<CIR::ConstNumber>(is_float, number_type, nb_val));
    //return CIR::InstructionRef();
}

CIR::InstructionRef UnaryExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef NullExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef ClosureAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef CastExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef CommentExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

void ModAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){

}

void MembersDeclarAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){

}

void EnumDeclarAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){

}

void UnionDeclarAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){

}

void GlobalVariableAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){

}

void FunctionAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto proto = std::make_unique<CIR::FunctionProto>(Proto->Name, Proto->cpoint_type, Proto->Args);
    auto function = std::make_unique<CIR::Function>(std::move(proto));
    fileCIR->add_function(std::move(function));
    fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>("entry"));
    for (int i = 0; i < Body.size(); i++){
        Body.at(i)->cir_gen(fileCIR);
    }
    //fileCIR->functions.push_back(std::move(function));
}

void StructDeclarAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){

}

std::unique_ptr<FileCIR> FileAST::cir_gen(){
  auto fileCIR = std::make_unique<FileCIR>(std::vector<std::unique_ptr<CIR::Function>>());
  for (int i = 0; i < global_vars.size(); i++){
    global_vars.at(i)->cir_gen(fileCIR);
  }
  /*for (int i = 0; i < function_protos.size(); i++){
    function_protos.at(i)->cir_gen();
  }*/
  for (int i = 0; i < structs.size(); i++){
    structs.at(i)->cir_gen(fileCIR);
  }
  for (int i = 0; i < enums.size(); i++){
    enums.at(i)->cir_gen(fileCIR);
  }
  for (int i = 0; i < unions.size(); i++){
    unions.at(i)->cir_gen(fileCIR);
  }
  for (int i = 0; i < members.size(); i++){
    members.at(i)->cir_gen(fileCIR);
  }
  for (int i = 0; i < functions.size(); i++){
    functions.at(i)->cir_gen(fileCIR);
  }
  for (int i = 0; i < mods.size(); i++){
    mods.at(i)->cir_gen(fileCIR);
  }
  return fileCIR;
}

#endif