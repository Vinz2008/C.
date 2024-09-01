//#include "config.h"
#include "cir.h"

#if ENABLE_CIR

#include "../ast.h"
#include "../reflection.h"

// TODO verify that last instruction of function is never type (a return, an unreachable, etc)

static CIR::InstructionRef codegenBody(std::unique_ptr<FileCIR>& fileCIR, std::vector<std::unique_ptr<ExprAST>>& Body){
    CIR::InstructionRef ret;
    for (int i = 0; i < Body.size(); i++){
        ret = Body.at(i)->cir_gen(fileCIR);
        // TODO : readd this when all exprs are implemented ?
        /*if (ret.is_empty()){
            return ret;
        }*/
    }
    return ret;
}

static CIR::InstructionRef createGoto(std::unique_ptr<FileCIR>& fileCIR, CIR::BasicBlockRef goto_basic_block){
    fileCIR->get_basic_block(goto_basic_block)->predecessors.push_back(fileCIR->CurrentBasicBlock);
    auto goto_instr = std::make_unique<CIR::GotoInstruction>(goto_basic_block);
    goto_instr->type = Cpoint_Type(never_type);
    return fileCIR->add_instruction(std::move(goto_instr));
}

CIR::InstructionRef VariableExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return fileCIR->CurrentFunction->vars[Name];
}

CIR::InstructionRef DeferExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef ReturnAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto return_inst = std::make_unique<CIR::ReturnInstruction>(returned_expr->cir_gen(fileCIR));
    return_inst->type = Cpoint_Type(never_type);
    return fileCIR->add_instruction(std::move(return_inst));
}

CIR::InstructionRef ScopeExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    // add createScope and endScope to make defers work
    CIR::InstructionRef ret = codegenBody(fileCIR, Body);
    return ret;
}

CIR::InstructionRef AsmExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}


CIR::InstructionRef AddrExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    // TODO : implement GEP in the language ?
    return CIR::InstructionRef();
}

CIR::InstructionRef VarExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::InstructionRef varInit;
    if (VarNames.at(0).second){
        varInit = VarNames.at(0).second->cir_gen(fileCIR);
        if (!varInit.is_empty()){ // TODO : remove this if ?
            CIR::Instruction* init_instr = fileCIR->get_instruction(varInit);
            if (init_instr->type != cpoint_type){
                if (!cir_convert_to_type(fileCIR, init_instr->type, cpoint_type, varInit)){
                    return CIR::InstructionRef();
                }
            }
        }
    }
    auto var_instr = std::make_unique<CIR::VarInit>(std::make_pair(VarNames.at(0).first, varInit), cpoint_type);
    var_instr->type = cpoint_type;
    auto var_ref = fileCIR->add_instruction(std::move(var_instr));
    fileCIR->CurrentFunction->vars[VarNames.at(0).first] = var_ref;
    return var_ref;
    //return CIR::InstructionRef();
}

CIR::InstructionRef EmptyExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef StringExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    fileCIR->strings.push_back(str);
    auto load_global_instr = std::make_unique<CIR::LoadGlobalInstruction>(true, fileCIR->strings.size()-1);
    load_global_instr->type = Cpoint_Type(i8_type, true);
    return fileCIR->add_instruction(std::move(load_global_instr));
}

CIR::InstructionRef DerefExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef IfExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef TypeidExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    int typeId = getTypeId(this->val->get_type());
    CIR::ConstNumber::nb_val_ty nb_val;
    nb_val.int_nb = typeId;
    Cpoint_Type nb_type = Cpoint_Type(i32_type);
    auto typeid_instr = std::make_unique<CIR::ConstNumber>(false, nb_type, nb_val);
    typeid_instr->type = nb_type;
    return fileCIR->add_instruction(std::move(typeid_instr));
}

CIR::InstructionRef CharExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::ConstNumber::nb_val_ty nb_val;
    nb_val.int_nb = c;
    Cpoint_Type char_type = Cpoint_Type(i8_type);
    auto const_char_instr = std::make_unique<CIR::ConstNumber>(false, char_type, nb_val);
    const_char_instr->type = char_type;
    return fileCIR->add_instruction(std::move(const_char_instr));
}

CIR::InstructionRef SemicolonExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto void_instr = std::make_unique<CIR::ConstVoid>();
    void_instr->type = Cpoint_Type(void_type);
    return fileCIR->add_instruction(std::move(void_instr));
}

CIR::InstructionRef SizeofExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return fileCIR->add_instruction(std::make_unique<CIR::SizeofInstruction>(is_type, type, (is_type) ? CIR::InstructionRef() : expr->cir_gen(fileCIR)));
}

CIR::InstructionRef BoolExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::ConstNumber::nb_val_ty nb_val;
    nb_val.int_nb = (val) ? 1 : 0;
    return fileCIR->add_instruction(std::make_unique<CIR::ConstNumber>(false, Cpoint_Type(bool_type), nb_val));
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
    call_instr->type = this->get_type();
    for (int i = 0; i < Args.size(); i++){
        call_instr->Args.push_back(Args.at(i)->cir_gen(fileCIR));
    }
    return fileCIR->add_instruction(std::move(call_instr));
}

CIR::InstructionRef GotoExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto goto_basic_block = fileCIR->get_basic_block_from_name(label_name);
    assert(!goto_basic_block.is_empty());
    return createGoto(fileCIR, goto_basic_block);
    //return fileCIR->add_instruction(std::make_unique<CIR::GotoInstruction>(goto_basic_block));
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
    CIR::BasicBlockRef labelBB = fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>(label_name));
    //fileCIR->add_instruction(std::make_unique<CIR::GotoInstruction>(labelBB));
    createGoto(fileCIR, labelBB);
    fileCIR->set_insert_point(labelBB);
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
    auto const_nb_instr = std::make_unique<CIR::ConstNumber>(is_float, number_type, nb_val);
    const_nb_instr->type = number_type;
    return fileCIR->add_instruction(std::move(const_nb_instr));
    //return CIR::InstructionRef();
}

CIR::InstructionRef UnaryExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef NullExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto null_instr = std::make_unique<CIR::Null>();
    null_instr->type = Cpoint_Type(void_type, true); // TODO : replace this with a null type than can be cast to any pointer in a CIR_Type
    return fileCIR->add_instruction(std::move(null_instr));
}

CIR::InstructionRef ClosureAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef CastExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::InstructionRef val_to_cast = ValToCast->cir_gen(fileCIR);
    
    //auto cast_instr = std::make_unique<CIR::CastInstruction>(val_to_cast, type);
    //cast_instr->type = type;
    //return fileCIR->add_instruction(std::move(cast_instr));
    if (!cir_convert_to_type(fileCIR, fileCIR->get_instruction(val_to_cast)->type, type, val_to_cast)){
        return CIR::InstructionRef();
    }

    return val_to_cast;

}

CIR::InstructionRef CommentExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

void ModAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    for (int i = 0; i < function_protos.size(); i++){
        std::unique_ptr<PrototypeAST> function_proto = function_protos.at(i)->clone(); // for now, clone this because it is used for the codegen function (TODO ?)
        std::string mangled_function_name = module_mangling(mod_name, function_proto->Name);
        function_proto->Name = mangled_function_name;
        function_proto->cir_gen(fileCIR);
    
        FunctionProtos[mangled_function_name] = function_proto->clone();
    }
    for (int i = 0; i < functions.size(); i++){
        std::unique_ptr<FunctionAST> function = functions.at(i)->clone();
        std::string mangled_function_name = module_mangling(mod_name, function->Proto->Name);
        function->Proto->Name = mangled_function_name;
        function->cir_gen(fileCIR);
    
        // functions.at(i)->Proto
    }
    for (int i = 0; i < structs.size(); i++){
        // TODO
        fprintf(stderr, "STRUCTS IN MOD NOT IMPLEMENTED YET\n");
        exit(1);
    }
    for (int i = 0; i < mods.size(); i++){
        std::unique_ptr<ModAST> mod = mods.at(i)->clone();
        mod->mod_name = module_mangling(mod_name, mod->mod_name);
        mod->cir_gen(fileCIR);
    }
}

void MembersDeclarAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    
}

void EnumDeclarAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){

}

void UnionDeclarAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){

}

void GlobalVariableAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){

}

void PrototypeAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    FunctionProtos[this->getName()] = this->clone();
}

void FunctionAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto proto = std::make_unique<CIR::FunctionProto>(Proto->Name, Proto->cpoint_type, Proto->Args);
    auto function = std::make_unique<CIR::Function>(std::move(proto));
    fileCIR->add_function(std::move(function));
    CIR::BasicBlockRef entry_bb = fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>("entry"));
    fileCIR->set_insert_point(entry_bb);
    for (int i = 0; i < Proto->Args.size(); i++){
        auto load_arg_instr = std::make_unique<CIR::LoadArgInstruction>(Proto->Args.at(i).first, Proto->Args.at(i).second);
        load_arg_instr->type = Proto->Args.at(i).second;
        auto load_arg = fileCIR->add_instruction(std::move(load_arg_instr));
        fileCIR->CurrentFunction->vars[Proto->Args.at(i).first] = load_arg;
    }
    CIR::InstructionRef ret_val;
    for (int i = 0; i < Body.size(); i++){
        ret_val = Body.at(i)->cir_gen(fileCIR);
    }
    if (Proto->cpoint_type.type == void_type && !Proto->cpoint_type.is_ptr){
        auto void_instr = std::make_unique<CIR::ConstVoid>();
        void_instr->type = Cpoint_Type(void_type);
        ret_val = fileCIR->add_instruction(std::move(void_instr));
    } else if (Proto->Name == "main") {
        CIR::ConstNumber::nb_val_ty nb_val;
        nb_val.int_nb = 0;
        Cpoint_Type main_ret_type = Cpoint_Type(i32_type);
        auto main_ret_instr = std::make_unique<CIR::ConstNumber>(false, main_ret_type, nb_val);
        main_ret_instr->type = main_ret_type;
        ret_val = fileCIR->add_instruction(std::move(main_ret_instr));
    }
    auto return_instr = std::make_unique<CIR::ReturnInstruction>(ret_val);
    return_instr->type = Cpoint_Type(never_type);
    fileCIR->add_instruction(std::move(return_instr));
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