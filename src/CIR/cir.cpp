//#include "config.h"
#include "cir.h"

#if ENABLE_CIR

#include "../cpoint.h"

#include <stack>
#include "../ast.h"
#include "../reflection.h"
#include "../tracy.h"

// TODO verify that last instruction of function (and basic block) is never type (a return, an unreachable, etc)
// TODO : add debug infos

// TODO : use this to remove duplicates global vars inits ?
bool operator==(const CIR::ConstInstruction& const1, const CIR::ConstInstruction& const2){
    if (dynamic_cast<const CIR::ConstNumber*>(&const1) && dynamic_cast<const CIR::ConstNumber*>(&const2)){
        auto const1nb = dynamic_cast<const CIR::ConstNumber*>(&const1);
        auto const2nb = dynamic_cast<const CIR::ConstNumber*>(&const2);
        bool is_enum_val_same = false;
        if (const1nb->is_float == const2nb->is_float){
            if (const1nb->is_float){
                is_enum_val_same = const1nb->nb_val.float_nb == const2nb->nb_val.float_nb;
            } else {
                is_enum_val_same = const1nb->nb_val.int_nb == const2nb->nb_val.int_nb;
            }
        }
        return is_enum_val_same && const1nb->type == const1nb->type;
    }
    if (dynamic_cast<const CIR::ConstVoid*>(&const1) && dynamic_cast<const CIR::ConstVoid*>(&const2)){
        return true;
    }
    if (dynamic_cast<const CIR::ConstNever*>(&const1) && dynamic_cast<const CIR::ConstNever*>(&const2)){
        return true;
    }
    if (dynamic_cast<const CIR::ConstNull*>(&const1) && dynamic_cast<const CIR::ConstNull*>(&const2)){
        return true;
    }
    return false;
}

CIR::BasicBlock::BasicBlock(FileCIR* fileCIR, std::string name, std::vector<std::unique_ptr<Instruction>> instructions) : name(name), instructions(std::move(instructions)), predecessors() {
    // TODO : remove the fileCIR arg and move this back to cir.h
    /*if (fileCIR->get_basic_block_from_name(name) != nullptr){
        name += std::to_string(fileCIR->already_named_index);
        fileCIR->already_named_index++;
    }*/
}

static Cpoint_Type cir_get_var_type(std::unique_ptr<FileCIR>& fileCIR, std::string var_name){
    if (!fileCIR->CurrentFunction->vars[var_name].is_empty()){
        return fileCIR->CurrentFunction->vars[var_name].type;
    }
    if (fileCIR->global_vars[var_name]){
        return fileCIR->global_vars[var_name]->type;
    }

    LogErrorV(emptyLoc, "Unknown Var : %s", var_name.c_str()); // should never happen, verify before (TODO) ?
    return Cpoint_Type();
}

// TODO : is this needed ?
Cpoint_Type CIR::InstructionRef::get_type(std::unique_ptr<FileCIR>& fileCIR){
    return fileCIR->CurrentFunction->get_instruction(this->get_pos())->type;
}

// TODO : add other externs (for tests, etc)
static void add_manually_extern(std::unique_ptr<FileCIR>& fileCIR, CIR::FunctionProto proto){
    fileCIR->protos[proto.name] = proto;
}

static void add_externs_for_gc(std::unique_ptr<FileCIR>& fileCIR){
    std::vector<std::pair<std::string, Cpoint_Type>> args_gc_init;
    add_manually_extern(fileCIR, CIR::FunctionProto("gc_init", Cpoint_Type(void_type), args_gc_init, false, false));
    std::vector<std::pair<std::string, Cpoint_Type>> args_gc_malloc;
    args_gc_malloc.push_back(std::make_pair("size", Cpoint_Type(i64_type)));
    add_manually_extern(fileCIR, CIR::FunctionProto("gc_malloc", Cpoint_Type(void_type, true), args_gc_malloc, false, false));
    std::vector<std::pair<std::string, Cpoint_Type>> args_gc_realloc;
    args_gc_realloc.push_back(std::make_pair("ptr", Cpoint_Type(void_type, true)));
    args_gc_realloc.push_back(std::make_pair("size", Cpoint_Type(i64_type)));
    add_manually_extern(fileCIR, CIR::FunctionProto("gc_realloc", Cpoint_Type(void_type, true), args_gc_realloc, false, false));
}


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

static CIR::InstructionRef createGoto(std::unique_ptr<FileCIR>& fileCIR, CIR::BasicBlock* goto_basic_block){
    //fileCIR->get_basic_block(goto_basic_block)->predecessors.push_back(fileCIR->CurrentBasicBlock);
    goto_basic_block->predecessors.push_back(fileCIR->CurrentBasicBlock);
    auto goto_instr = std::make_unique<CIR::GotoInstruction>(goto_basic_block);
    goto_instr->type = Cpoint_Type(never_type);
    return fileCIR->add_instruction(std::move(goto_instr));
}

static CIR::InstructionRef createGotoIf(std::unique_ptr<FileCIR>& fileCIR, CIR::InstructionRef CondI, CIR::BasicBlock* goto_bb_true, CIR::BasicBlock* goto_bb_false){
    //fileCIR->get_basic_block(goto_bb_true)->predecessors.push_back(fileCIR->CurrentBasicBlock);
    goto_bb_true->predecessors.push_back(fileCIR->CurrentBasicBlock);
    //fileCIR->get_basic_block(goto_bb_false)->predecessors.push_back(fileCIR->CurrentBasicBlock);
    goto_bb_false->predecessors.push_back(fileCIR->CurrentBasicBlock);
    auto goto_instr = std::make_unique<CIR::GotoIfInstruction>(CondI, goto_bb_true, goto_bb_false);
    goto_instr->type = Cpoint_Type(never_type);
    return fileCIR->add_instruction(std::move(goto_instr));
}

CIR::InstructionRef VariableExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    if (fileCIR->global_vars[Name]){
        auto load_global_instr = std::make_unique<CIR::LoadGlobalInstruction>(fileCIR->global_vars[Name]->type, false, -1, Name);
        load_global_instr->label = Name + ".load_global";  
        return fileCIR->add_instruction(std::move(load_global_instr));
    }
    auto var_ref = fileCIR->CurrentFunction->vars[Name].var_ref;
    auto load_var_instr = std::make_unique<CIR::LoadVarInstruction>(var_ref, fileCIR->CurrentFunction->vars[Name].type);
    load_var_instr->label = Name + ".load";
    //load_var_instr->type = fileCIR->CurrentFunction->vars[Name].type;
    return fileCIR->add_instruction(std::move(load_var_instr));
    //return fileCIR->CurrentFunction->vars[Name].var_ref;
}

static std::deque<Scope> Scopes; // is there because it is not needed in the fileCIR, it is just needed for the desugaring

static void createScope(){
    Scope new_scope = Scope(std::deque<std::unique_ptr<ExprAST>>(), nullptr);
    Scopes.push_back(std::move(new_scope));
}

static void endScope(std::unique_ptr<FileCIR>& fileCIR){
    Scope back = std::move(Scopes.back());
    Log::Info() << "gen scope defers : " << back.to_string() << "\n";
    for (auto it = back.deferExprs.begin(); it != back.deferExprs.end(); ++it) {
        (*it)->cir_gen(fileCIR);
    }
    Scopes.pop_back();
}


CIR::InstructionRef DeferExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    Scope back = std::move(Scopes.back());
    Scopes.pop_back();
    back.deferExprs.push_back(std::move(Expr));
    Scopes.push_back(std::move(back));
    return fileCIR->add_instruction(std::make_unique<CIR::ConstVoid>());
}

CIR::InstructionRef ReturnAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto returnedI = returned_expr->cir_gen(fileCIR);
    Cpoint_Type returned_expr_type = returned_expr->get_type();
    if (returned_expr_type != fileCIR->CurrentFunction->proto->return_type){
        cir_convert_to_type(fileCIR, returned_expr_type, fileCIR->CurrentFunction->proto->return_type, returnedI);
    }
    auto return_inst = std::make_unique<CIR::ReturnInstruction>(returnedI);
    return_inst->type = Cpoint_Type(never_type);
    return fileCIR->add_instruction(std::move(return_inst));
}

CIR::InstructionRef ScopeExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    createScope();
    CIR::InstructionRef ret = codegenBody(fileCIR, Body);
    endScope(fileCIR);
    return ret;
}

CIR::InstructionRef AsmExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    for (int i = 0; i < InputOutputArgs.size(); i++){
        if (dynamic_cast<VariableExprAST*>(InputOutputArgs.at(i).ArgExpr.get())){
            auto var_expr = get_Expr_from_ExprAST<VariableExprAST>(InputOutputArgs.at(i).ArgExpr->clone());
            // TODO
        } else {
            LogErrorV(this->loc, "Unknown expression for \"in\" in asm macro"); // TODO : add dedicated function for this case
            return CIR::InstructionRef();
        }
    }
    return CIR::InstructionRef();
}


CIR::InstructionRef AddrExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    // TODO : implement GEP in the language ?
    if (dynamic_cast<VariableExprAST*>(Expr.get())){
        std::unique_ptr<VariableExprAST> VariableExpr = get_Expr_from_ExprAST<VariableExprAST>(Expr->clone());
        Log::Info() << "Addr Variable : " << VariableExpr->getName() << "\n";
        if (fileCIR->global_vars[VariableExpr->Name]){
            NOT_IMPLEMENTED();
        } else {
            return fileCIR->CurrentFunction->vars[VariableExpr->Name].var_ref;
        }
    } else if (dynamic_cast<BinaryExprAST*>(Expr.get())){
        NOT_IMPLEMENTED();
    }
    return CIR::InstructionRef();
}

CIR::InstructionRef VarExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::InstructionRef varInit;
    if (VarNames.at(0).second){
        varInit = VarNames.at(0).second->cir_gen(fileCIR);
        if (!varInit.is_empty()){ // TODO : remove this if ?
            Cpoint_Type init_type = VarNames.at(0).second->get_type();
            if (infer_type){
                cpoint_type = init_type;
            } else {
            if (init_type != cpoint_type){
                if (!cir_convert_to_type(fileCIR, init_type, cpoint_type, varInit)){
                    return CIR::InstructionRef();
                }
            }
            }
        }
    }
    auto var_instr = std::make_unique<CIR::VarInit>(std::make_pair(VarNames.at(0).first, varInit), cpoint_type);
    var_instr->type = cpoint_type;
    var_instr->label = VarNames.at(0).first;
    auto var_ref = fileCIR->add_instruction(std::move(var_instr));
    fileCIR->CurrentFunction->vars[VarNames.at(0).first] = CIR::Var(var_ref, cpoint_type);
    // TODO : struct constructors
    // TODO : debuginfos
    return var_ref;
    //return CIR::InstructionRef();
}

CIR::InstructionRef EmptyExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef StringExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto find_duplicate_string = std::find(fileCIR->strings.begin(), fileCIR->strings.end(), str);
    int pos = -1;
    if (find_duplicate_string != fileCIR->strings.end()){
        pos = std::distance(fileCIR->strings.begin(), find_duplicate_string);
    } else {
        fileCIR->strings.push_back(str);
        pos = fileCIR->strings.size()-1;
    }
    auto load_global_instr = std::make_unique<CIR::LoadGlobalInstruction>(Cpoint_Type(i8_type, true), true, pos, "");
    return fileCIR->add_instruction(std::move(load_global_instr));
}

CIR::InstructionRef DerefExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    if (dynamic_cast<VariableExprAST*>(Expr.get())){
        std::unique_ptr<VariableExprAST> VariableExpr = get_Expr_from_ExprAST<VariableExprAST>(Expr->clone());
        // TODO : make this work with global vars (add functions like in codegen.cpp to known if the var exists and have the var ptr andthe var type)
        if (fileCIR->CurrentFunction->vars[VariableExpr->Name].is_empty()){
            LogErrorV(this->loc, "Unknown variable in deref : %s", VariableExpr->getName().c_str());
            return CIR::InstructionRef();
        }
        if (!fileCIR->CurrentFunction->vars[VariableExpr->Name].type.is_ptr){
            LogErrorV(this->loc, "Derefencing a variable (%s) that is not a pointer", VariableExpr->getName().c_str());
            return CIR::InstructionRef();
        }

        CIR::InstructionRef varAlloc = fileCIR->CurrentFunction->vars[VariableExpr->Name].var_ref;
        Cpoint_Type contained_type = fileCIR->CurrentFunction->vars[VariableExpr->Name].type.deref_type();

        Log::Info() << "Deref Type : " << contained_type << "\n";
        return fileCIR->add_instruction(std::make_unique<CIR::DerefInstruction>(varAlloc, contained_type));
        //return Builder->CreateLoad(get_type_llvm(contained_type), VarAlloc, VariableExpr->getName().c_str());
    }
    NOT_IMPLEMENTED();
    return CIR::InstructionRef();
}

CIR::InstructionRef IfExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    Cpoint_Type cond_type = Cond->get_type(fileCIR.get());
    CIR::InstructionRef CondI = Cond->cir_gen(fileCIR);
    if (CondI.is_empty()){
        return CIR::InstructionRef();
    }
    if (cond_type.type != bool_type || cond_type.is_vector_type){
        // TODO : create default comparisons : if is pointer, compare to null, if number compare to 1, etc
        cir_convert_to_type(fileCIR, cond_type, Cpoint_Type(bool_type), CondI);
    }

    bool has_one_branch_if = false;
    CIR::BasicBlock* ThenBB = fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>(fileCIR.get(), "then"));
    std::unique_ptr<CIR::BasicBlock> ElseBB = std::make_unique<CIR::BasicBlock>(fileCIR.get(), "else");
    std::unique_ptr<CIR::BasicBlock> MergeBB = std::make_unique<CIR::BasicBlock>(fileCIR.get(), "ifcont");
    createGotoIf(fileCIR, CondI, ThenBB, ElseBB.get());
    fileCIR->set_insert_point(ThenBB);
    bool has_then_return = Then->contains_expr(ExprType::Return);
    bool has_then_break = Then->contains_expr(ExprType::Break);
    bool has_then_unreachable = Then->contains_expr(ExprType::Unreachable);
    bool has_then_never_function_call = Then->contains_expr(ExprType::NeverFunctionCall);
    CIR::InstructionRef ThenI = Then->cir_gen(fileCIR);
    if (!has_then_break /*!break_found*/ && !has_then_return && !has_then_unreachable && !has_then_never_function_call){
        createGoto(fileCIR, MergeBB.get());
    } else {
        has_one_branch_if = true;
    }

    // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
    ThenBB = fileCIR->CurrentBasicBlock;
    CIR::BasicBlock* ElseBBPtr = ElseBB.get();
    fileCIR->add_basic_block(std::move(ElseBB));
    fileCIR->set_insert_point(ElseBBPtr);
    //Cpoint_Type phi_type = Then->get_type();
    
    // TODO : replace the phitype from the function return type to the type of the Then block
    //Cpoint_Type phi_type = fileCIR->CurrentFunction->proto->return_type;

    Cpoint_Type Then_type = Then->get_type(fileCIR.get());
    /*if (Then_type != Cpoint_Type(void_type)){
        phi_type = Then_type;
    }*/
   Cpoint_Type phi_type = Then_type;
    
    bool has_else_return = false;
    bool has_else_break = false;
    bool has_else_unreachable = false;
    bool has_else_never_function_call = false;
    CIR::InstructionRef ElseI;
    if (Else){
        has_else_return = Else->contains_expr(ExprType::Return);
        has_else_break = Else->contains_expr(ExprType::Break);
        has_else_unreachable = Else->contains_expr(ExprType::Unreachable);
        has_else_never_function_call = Else->contains_expr(ExprType::NeverFunctionCall);
        ElseI = Else->cir_gen(fileCIR);
        if (ElseI.is_empty()){
            return CIR::InstructionRef();
        }
        Cpoint_Type Else_type = Else->get_type(fileCIR.get());
        //std::cout << "Else_type : " << Else_type << std::endl;
        //std::cout << "phi_type : " << phi_type << std::endl;
        if (Else_type.type != void_type && Else_type != phi_type){
            if (!cir_convert_to_type(fileCIR, Else_type, phi_type, ElseI)){
                // TODO: add special function for errors in this context
                // TODO : verify if this work if the Then is void and the Else is another type(ex : int)
                LogErrorV(this->loc, "Mismatch, expected : %s, got : %s", phi_type.c_stringify(), Else_type.c_stringify());
                return CIR::InstructionRef();
            }
        }
    }

    if (!has_else_break && !has_else_return && !has_else_unreachable && !has_else_never_function_call){
        createGoto(fileCIR, MergeBB.get());
    } else {
        has_one_branch_if = true;
    }
    // TODO : add the GetInsertBlock to this ?

    CIR::BasicBlock* MergeBBPtr = MergeBB.get();
    fileCIR->add_basic_block(std::move(MergeBB));
    fileCIR->set_insert_point(MergeBBPtr);

    /*if (fileCIR->CurrentFunction->proto->return_type.type == void_type){
        return fileCIR->add_instruction(std::make_unique<CIR::ConstVoid>());
    }*/

    if (Else && !has_one_branch_if && phi_type != Cpoint_Type(void_type)){
        CIR::BasicBlock* phi_bb1 = ThenBB;
        CIR::InstructionRef phi_arg1 = ThenI;
        CIR::BasicBlock* phi_bb2 = ElseBBPtr;
        CIR::InstructionRef phi_arg2 = ElseI;
        return fileCIR->add_instruction(std::make_unique<CIR::PhiInstruction>(phi_type, phi_bb1, phi_arg1, phi_bb2, phi_arg2));
    }
    return fileCIR->add_instruction(std::make_unique<CIR::ConstVoid>());
}

CIR::InstructionRef TypeidExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    int typeId = getTypeId(this->val->get_type(fileCIR.get()));
    CIR::ConstNumber::nb_val_ty nb_val;
    nb_val.int_nb = typeId;
    Cpoint_Type nb_type = Cpoint_Type(i32_type);
    auto typeid_instr = std::make_unique<CIR::ConstNumber>(nb_type, false, nb_val);
    return fileCIR->add_instruction(std::move(typeid_instr));
}

CIR::InstructionRef CharExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::ConstNumber::nb_val_ty nb_val;
    nb_val.int_nb = c;
    Cpoint_Type char_type = Cpoint_Type(i8_type);
    auto const_char_instr = std::make_unique<CIR::ConstNumber>(char_type, false, nb_val);
    return fileCIR->add_instruction(std::move(const_char_instr));
}

CIR::InstructionRef SemicolonExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto void_instr = std::make_unique<CIR::ConstVoid>();
    return fileCIR->add_instruction(std::move(void_instr));
}

CIR::InstructionRef SizeofExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return fileCIR->add_instruction(std::make_unique<CIR::SizeofInstruction>(is_type, type, (is_type) ? CIR::InstructionRef() : expr->cir_gen(fileCIR)));
}

CIR::InstructionRef BoolExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto bool_instr = std::make_unique<CIR::ConstBool>(val);
    return fileCIR->add_instruction(std::move(bool_instr));
}

static CIR::InstructionRef InfiniteLoopCodegen(std::unique_ptr<FileCIR>& fileCIR, std::vector<std::unique_ptr<ExprAST>> &Body){
    CIR::BasicBlock* loopBB = fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>(fileCIR.get(), "loop_infinite"));
    createGoto(fileCIR, loopBB);
    fileCIR->set_insert_point(loopBB);
    createScope();
    for (int i = 0; i < Body.size(); i++){
      if (Body.at(i)->cir_gen(fileCIR).is_empty())
        return CIR::InstructionRef();
    }
    endScope(fileCIR);
    createGoto(fileCIR, loopBB);
    //fileCIR->add_instruction(std::make_unique<CIR::ConstNever>());
    //fileCIR->set_insert_point(loopBB);

    return fileCIR->add_instruction(std::make_unique<CIR::ConstNever>());
}

static std::stack<CIR::BasicBlock*> blocksForBreak;

CIR::InstructionRef LoopExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    if (is_infinite_loop){
        return InfiniteLoopCodegen(fileCIR, Body);
    } else {
        // TODO 
        // add createScope and blocksForBreak.push
    }
    return CIR::InstructionRef();
}

CIR::InstructionRef ForExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::InstructionRef StartI = Start->cir_gen(fileCIR);
    if (StartI.is_empty()){
        return CIR::InstructionRef();
    }
    Cpoint_Type StartType = Start->get_type(fileCIR.get());
    if (StartType != VarType){
        cir_convert_to_type(fileCIR, StartType, VarType, StartI);
    }
    auto var_instr = fileCIR->add_instruction(std::make_unique<CIR::VarInit>(std::make_pair(VarName, StartI), VarType));
    fileCIR->CurrentFunction->vars[VarName] = CIR::Var(var_instr, VarType);
    // TODO : save OldVal (see codegen.cpp)

    CIR::BasicBlock* CondBB = fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>(fileCIR.get(), "loop_for_cond"));
    createGoto(fileCIR, CondBB);
    fileCIR->set_insert_point(CondBB);

    CIR::InstructionRef EndCondI = End->cir_gen(fileCIR);
    if (EndCondI.is_empty()){
        return CIR::InstructionRef();
    }
    CIR::BasicBlock* LoopBB = fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>(fileCIR.get(), "loop_for"));
    std::unique_ptr<CIR::BasicBlock> AfterBB = std::make_unique<CIR::BasicBlock>(fileCIR.get(), "afterloop");
    createGotoIf(fileCIR, EndCondI, LoopBB, AfterBB.get());
    fileCIR->set_insert_point(LoopBB);
    blocksForBreak.push(AfterBB.get());
    createScope();
    // TODO : break, scopes and debuginfos support (see codegen.cpp)
    //auto afterBBpoped = fileCIR->CurrentFunction->pop_bb();
    CIR::InstructionRef lastI = Body->cir_gen(fileCIR);
    if (lastI.is_empty()){
        return CIR::InstructionRef();
    }
    endScope(fileCIR);
    blocksForBreak.pop();
    CIR::InstructionRef StepI;
    if (Step){
        Cpoint_Type StepType = Step->get_type(fileCIR.get());
        StepI = Step->cir_gen(fileCIR);
        if (StepI.is_empty()){
            return CIR::InstructionRef();
        }

        if (StepType != VarType){
            cir_convert_to_type(fileCIR, StepType, VarType, StepI);
        }

    } else {
        bool is_float = VarType.is_decimal_number_type();
        CIR::ConstNumber::nb_val_ty nb_val;
        if (is_float){
            nb_val.float_nb = 1.0;
        } else {
            nb_val.int_nb = 1;
        }
        StepI = fileCIR->add_instruction(std::make_unique<CIR::ConstNumber>(VarType, is_float, nb_val));
    }
    CIR::InstructionRef CurVarI = fileCIR->add_instruction(std::make_unique<CIR::LoadVarInstruction>(var_instr, VarType));
    CIR::InstructionRef NextVarI = fileCIR->add_instruction(std::make_unique<CIR::AddInstruction>(VarType, CurVarI, StepI));
    fileCIR->add_instruction(std::make_unique<CIR::StoreVarInstruction>(var_instr, NextVarI));
    createGoto(fileCIR, CondBB);

    //fileCIR->CurrentFunction->push_bb(std::move(afterBBpoped));
    
    auto AfterBBPtr = AfterBB.get();
    fileCIR->add_basic_block(std::move(AfterBB));
    fileCIR->set_insert_point(AfterBBPtr);
    return fileCIR->add_instruction(std::make_unique<CIR::ConstVoid>());
}

CIR::InstructionRef StructMemberCallExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef MatchExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef ConstantVectorExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    std::vector<CIR::InstructionRef> VectorMembersI;
    for (int i = 0; i < VectorMembers.size(); i++){
        CIR::InstructionRef instr = VectorMembers.at(i)->cir_gen(fileCIR);
        if (!dynamic_cast<CIR::ConstInstruction*>(fileCIR->get_instruction(instr))){
            LogErrorV(this->loc, "Passing a non constant value to a constant vector in %d pos", i);
            return CIR::InstructionRef();
        }
        VectorMembersI.push_back(instr);
    }
    return fileCIR->add_instruction(std::make_unique<CIR::ConstVector>(vector_element_type, VectorMembersI));
}

CIR::InstructionRef EnumCreation::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef CallExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    // for debug for new

    // TODO : implement internal functions

    std::string internal_func_prefix = INTERNAL_FUNC_PREFIX;
    if (Callee.rfind(internal_func_prefix, 0) == 0){
        std::string internal_func = Callee.substr(internal_func_prefix.size(), Callee.size());
        // TODO : move this to CallExprAST parsing ?
        if (internal_func == "get_filename"){
            std::string filename_without_temp = fileCIR->filename;
            return StringExprAST(filename_without_temp).cir_gen(fileCIR);  
        }
        if (internal_func == "get_function_name"){
            return StringExprAST(fileCIR->CurrentFunction->proto->name).cir_gen(fileCIR);
        }
        if (internal_func == "unreachable"){
            return fileCIR->add_instruction(std::make_unique<CIR::ConstNever>());
        }
        // TODO
    }

    auto call_instr = std::make_unique<CIR::CallInstruction>(this->get_type(fileCIR.get()), Callee, std::vector<CIR::InstructionRef>());
    for (int i = 0; i < Args.size(); i++){
        CIR::InstructionRef argI = Args.at(i)->cir_gen(fileCIR);
        Cpoint_Type arg_type = Args.at(i)->get_type(fileCIR.get());
        //std::cout << fileCIR->protos[Callee].args.size() << std::endl;
        if (arg_type != fileCIR->protos[Callee].args.at(i).second){
            cir_convert_to_type(fileCIR, arg_type, fileCIR->protos[Callee].args.at(i).second, argI);
        }
        call_instr->Args.push_back(argI);
    }
    return fileCIR->add_instruction(std::move(call_instr));
}

CIR::InstructionRef GotoExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto goto_basic_block = fileCIR->get_basic_block_from_name(label_name);
    assert(!goto_basic_block);
    return createGoto(fileCIR, goto_basic_block);
    //return fileCIR->add_instruction(std::make_unique<CIR::GotoInstruction>(goto_basic_block));
}

CIR::InstructionRef WhileExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::BasicBlock* whileBB = fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>(fileCIR.get(), "while"));
    createGoto(fileCIR, whileBB);
    fileCIR->set_insert_point(whileBB);
    CIR::BasicBlock* LoopBB = fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>(fileCIR.get(), "loop_while"));
    CIR::InstructionRef CondI = Cond->cir_gen(fileCIR);
    if (CondI.is_empty()){
        return CIR::InstructionRef();
    }
    CIR::BasicBlock* AfterBB = fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>(fileCIR.get(), "afterloop_while"));
    // TODO : add to block for breaks

    blocksForBreak.push(AfterBB);

    createGotoIf(fileCIR, CondI, LoopBB, AfterBB);
    fileCIR->set_insert_point(LoopBB);

    createScope();

    Body->cir_gen(fileCIR);

    endScope(fileCIR);
    blocksForBreak.pop();

    createGoto(fileCIR, whileBB);
    fileCIR->set_insert_point(AfterBB);
    return fileCIR->add_instruction(std::make_unique<CIR::ConstNever>());
    //return CIR::InstructionRef();
}

CIR::InstructionRef BreakExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    if (blocksForBreak.empty()){
        LogErrorV(this->loc, "Break statement not in a a loop");
        return CIR::InstructionRef();
    }
    createGoto(fileCIR, blocksForBreak.top());
    return fileCIR->add_instruction(std::make_unique<CIR::ConstVoid>());
}

CIR::InstructionRef ConstantStructExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

// TODO : add a LogError for InstructionRef

extern std::unordered_map<std::string, int> BinopPrecedence;

static CIR::InstructionRef getArrayMember(std::unique_ptr<FileCIR>& fileCIR, std::unique_ptr<ExprAST> array, std::unique_ptr<ExprAST> index){
    NOT_IMPLEMENTED();
}

static CIR::InstructionRef equalOperator(std::unique_ptr<FileCIR>& fileCIR, std::unique_ptr<ExprAST> lvalue, std::unique_ptr<ExprAST> rvalue){
    if (dynamic_cast<BinaryExprAST*>(lvalue.get())){
        Log::Info() << "Equal op bin lvalue" << "\n";
        std::unique_ptr<BinaryExprAST> BinExpr = get_Expr_from_ExprAST<BinaryExprAST>(std::move(lvalue));
        Log::Info() << "op : " << BinExpr->Op << "\n";
        if (BinExpr->Op.at(0) == '['){
            NOT_IMPLEMENTED();
        } else if (BinExpr->Op.at(0) == '.'){
            NOT_IMPLEMENTED();
        } else {
            NOT_IMPLEMENTED();
        }
    } else if (dynamic_cast<VariableExprAST*>(lvalue.get())){
        std::unique_ptr<VariableExprAST> VarExpr = get_Expr_from_ExprAST<VariableExprAST>(std::move(lvalue));
        bool is_global = fileCIR->global_vars[VarExpr->Name] != nullptr;
        Cpoint_Type var_type = cir_get_var_type(fileCIR, VarExpr->Name);
        Cpoint_Type rvalue_type = rvalue->get_type();
        CIR::InstructionRef rvalueI = rvalue->cir_gen(fileCIR);
        if (rvalue_type != var_type){
            cir_convert_to_type(fileCIR, rvalue_type, var_type, rvalueI);
        }
        if (is_global){
            return fileCIR->add_instruction(std::make_unique<CIR::StoreGlobalInstruction>(VarExpr->Name, rvalueI));
        } else {
            return fileCIR->add_instruction(std::make_unique<CIR::StoreVarInstruction>(fileCIR->CurrentFunction->vars[VarExpr->Name].var_ref, rvalueI));
        }
    } else if (dynamic_cast<UnaryExprAST*>(lvalue.get()) || dynamic_cast<DerefExprAST*>(lvalue.get())){
        CIR::InstructionRef lvalI;
        if (dynamic_cast<UnaryExprAST*>(lvalue.get())){
            std::unique_ptr<UnaryExprAST> UnaryExpr = get_Expr_from_ExprAST<UnaryExprAST>(std::move(lvalue));
            if (UnaryExpr->Opcode != '*'){
                LogErrorV(emptyLoc, "The equal operator is not implemented for other Unary Operators as rvalue than deref");
                return CIR::InstructionRef();
            }
            lvalI = AddrExprAST(std::move(UnaryExpr->Operand)).cir_gen(fileCIR);
        } else if (dynamic_cast<DerefExprAST*>(lvalue.get())){
            lvalI = lvalue->cir_gen(fileCIR);
        }
        // TODO
        NOT_IMPLEMENTED();
    }
    
    NOT_IMPLEMENTED();
}

static CIR::InstructionRef getStructMember(std::unique_ptr<FileCIR>& fileCIR, std::unique_ptr<ExprAST> struct_expr, std::unique_ptr<ExprAST> member){
    NOT_IMPLEMENTED();
}

CIR::InstructionRef BinaryExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    if (BinopPrecedence.find(Op) == BinopPrecedence.end()){ // is custom op
        std::vector<std::unique_ptr<ExprAST>> Args;
        Args.push_back(LHS->clone());
        Args.push_back(RHS->clone());
        return CallExprAST(emptyLoc, "binary" + Op, std::move(Args), Cpoint_Type()).cir_gen(fileCIR);
    }

    if (Op.at(0) == '['){
        return getArrayMember(fileCIR, LHS->clone(), RHS->clone());
    } 

    if (Op.at(0) == '=' && Op.size() == 1){
        return equalOperator(fileCIR, LHS->clone(), RHS->clone());
    }
    if (Op.at(0) == '.'){
        return getStructMember(fileCIR, LHS->clone(), RHS->clone());
    }


    // TODO : readd this after fixing get_type for vars (problem with NamedValues, put it in the AST node ?)
    Cpoint_Type LHS_type = LHS->get_type(fileCIR.get()).get_real_type();
    Cpoint_Type RHS_type = RHS->get_type(fileCIR.get());
    Cpoint_Type op_type = this->get_type(fileCIR.get());
    CIR::InstructionRef LHS_Instr = LHS->cir_gen(fileCIR);
    CIR::InstructionRef RHS_Instr = RHS->cir_gen(fileCIR);
    if (Op == "+"){ // TODO : is this needed ?
        if (LHS_type.is_ptr){
            cir_convert_to_type(fileCIR, LHS_type, Cpoint_Type(i64_type), LHS_Instr);
            //LHS_Instr = fileCIR->add_instruction(std::make_unique<CIR::CastInstruction>(LHS_Instr, Cpoint_Type(i64_type)));
            LHS_type = Cpoint_Type(i64_type);
        }
        if (RHS_type.is_ptr){
            cir_convert_to_type(fileCIR, RHS_type, Cpoint_Type(i64_type), RHS_Instr);
            //RHS_Instr = fileCIR->add_instruction(std::make_unique<CIR::CastInstruction>(LHS_Instr, Cpoint_Type(i64_type)));
            RHS_type = Cpoint_Type(i64_type);
        }
    }
    // TODO : transform i8 and i16s to i32 ? (see codegen.cpp and see if it is needed)
    /*std::cout << "LHS_Type : " << LHS_type << std::endl;
    std::cout << "RHS_type : " << RHS_type << std::endl;
    std::cout << "LHS_Type == RHS_Type : " << (LHS_type == RHS_type) << std::endl;*/
    if (LHS_type != RHS_type){
        (Log::Warning(this->loc) << "Types are not the same for the binary operation '" << Op << "' to the " << create_pretty_name_for_type(LHS_type) << " and " << create_pretty_name_for_type(RHS_type) << " types" << "\n").end(); // TODO : add a better syntax for simple warnings (for ex a wrapper struct WarningSimple which would call end in destructor)
        cir_convert_to_type(fileCIR, RHS_type, LHS_type, RHS_Instr);
        RHS_type = LHS_type;
        //RHS_Instr = fileCIR->add_instruction(std::make_unique<CIR::CastInstruction>(RHS_Instr, LHS_type));
    }
    if (Op == "&&" || Op == "&"){
        if (LHS_type.is_decimal_number_type()){
            // TODO : replace i32_type by i64_type ?
            cir_convert_to_type(fileCIR, LHS_type, Cpoint_Type(i32_type), LHS_Instr);
        }
        if (RHS_type.is_decimal_number_type()){
            cir_convert_to_type(fileCIR, LHS_type, Cpoint_Type(i32_type), RHS_Instr);
        }
        auto and_instr = std::make_unique<CIR::AndInstruction>(op_type, LHS_Instr, RHS_Instr);
        and_instr->type = this->get_type(fileCIR.get());
        return fileCIR->add_instruction(std::move(and_instr));
    }
    std::unique_ptr<CIR::Instruction> binary_instr = nullptr;
    if (Op == "=="){
        binary_instr = std::make_unique<CIR::CmpInstruction>(CIR::CmpInstruction::CMP_EQ, LHS_Instr, RHS_Instr);
    } else if (Op == "!="){
        binary_instr = std::make_unique<CIR::CmpInstruction>(CIR::CmpInstruction::CMP_NOT_EQ, LHS_Instr, RHS_Instr);
    } else if (Op == "<="){
        binary_instr = std::make_unique<CIR::CmpInstruction>(CIR::CmpInstruction::CMP_LOWER_EQ, LHS_Instr, RHS_Instr);
    } else if (Op == ">="){
        binary_instr = std::make_unique<CIR::CmpInstruction>(CIR::CmpInstruction::CMP_GREATER_EQ, LHS_Instr, RHS_Instr);
    } else if (Op == ">>"){
        binary_instr = std::make_unique<CIR::ShiftInstruction>(op_type, CIR::ShiftInstruction::SHIFT_LEFT, LHS_Instr, RHS_Instr);
    } else if (Op == "<<"){
        binary_instr = std::make_unique<CIR::ShiftInstruction>(op_type, CIR::ShiftInstruction::SHIFT_RIGHT, LHS_Instr, RHS_Instr);
    } else {
    switch (Op.at(0)) {
        case '+':
            binary_instr = std::make_unique<CIR::AddInstruction>(op_type, LHS_Instr, RHS_Instr);
            break;
        case '-':
            binary_instr = std::make_unique<CIR::SubInstruction>(op_type, LHS_Instr, RHS_Instr);
            break;
        case '*':
            binary_instr = std::make_unique<CIR::MulInstruction>(op_type, LHS_Instr, RHS_Instr);
            break;
        case '%':
            binary_instr = std::make_unique<CIR::RemInstruction>(op_type, LHS_Instr, RHS_Instr);
            break;
        case '<':
            binary_instr = std::make_unique<CIR::CmpInstruction>(CIR::CmpInstruction::CMP_LOWER, LHS_Instr, RHS_Instr);
            break;
        case '/':
            binary_instr = std::make_unique<CIR::DivInstruction>(op_type, LHS_Instr, RHS_Instr);
            break;
        case '>':
            binary_instr = std::make_unique<CIR::CmpInstruction>(CIR::CmpInstruction::CMP_GREATER, LHS_Instr, RHS_Instr);
            break;
        case '|':
            binary_instr = std::make_unique<CIR::OrInstruction>(op_type, LHS_Instr, RHS_Instr);
        default:
            // TODO : maybe reactivate this
            //LogError("invalid binary operator");
            //return CIR::InstructionRef();
            break;
    }
    }
    if (binary_instr){
        return fileCIR->add_instruction(std::move(binary_instr));
    }
    // TODO : readd custom operator support ?


    return CIR::InstructionRef();
}

CIR::InstructionRef ConstantArrayExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    return CIR::InstructionRef();
}

CIR::InstructionRef LabelExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::BasicBlock* labelBB = fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>(fileCIR.get(), label_name));
    //fileCIR->add_instruction(std::make_unique<CIR::GotoInstruction>(labelBB));
    createGoto(fileCIR, labelBB);
    fileCIR->set_insert_point(labelBB);
    return CIR::InstructionRef();
}

CIR::InstructionRef NumberExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    Cpoint_Type number_type = this->get_type(fileCIR.get());
    union CIR::ConstNumber::nb_val_ty nb_val;
    bool is_float = number_type.type == double_type;
    if (is_float){ 
        nb_val.float_nb = Val; 
    } else { 
        nb_val.int_nb = (int)Val; 
    }
    // insert in insertpoint instead
    auto const_nb_instr = std::make_unique<CIR::ConstNumber>(number_type, is_float, nb_val);
    const_nb_instr->type = number_type;
    return fileCIR->add_instruction(std::move(const_nb_instr));
    //return CIR::InstructionRef();
}

CIR::InstructionRef UnaryExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    if (Opcode == '-'){
        auto LHS = std::make_unique<NumberExprAST>(0);
        auto RHS = std::move(Operand);
        return BinaryExprAST(this->loc, "-", std::move(LHS), std::move(RHS)).cir_gen(fileCIR);
    }
    if (Opcode == '&'){
        return AddrExprAST(std::move(Operand)).cir_gen(fileCIR);
    }
    if (Opcode == '*'){
        return DerefExprAST(std::move(Operand)).cir_gen(fileCIR);
    }
    std::vector<std::unique_ptr<ExprAST>> Operands;
    Operands.push_back(Operand->clone());
    std::string unary_func = "unary";
    unary_func.push_back(Opcode);
    return CallExprAST(this->loc, unary_func, std::move(Operands), Cpoint_Type()).cir_gen(fileCIR);
}

CIR::InstructionRef NullExprAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto null_instr = std::make_unique<CIR::ConstNull>();
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

static bool is_const_instruction(CIR::Instruction* instr){
    return dynamic_cast<CIR::ConstInstruction*>(instr);
}

void GlobalVariableAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    CIR::InstructionRef InitI;
    if (Init){
        InitI = Init->cir_gen(fileCIR);
        if (!is_const_instruction(fileCIR->get_instruction(InitI))){
            // TODO : error
        }
    }
    fileCIR->global_vars[varName] = std::make_unique<CIR::GlobalVar>(varName, cpoint_type, is_const, is_extern, InitI, section_name, fileCIR->global_context.basicBlocks.size()-1);
}

void PrototypeAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    FunctionProtos[this->getName()] = this->clone();
    auto proto_clone = CIR::FunctionProto(this->Name, this->cpoint_type, this->Args, this->is_variable_number_args, this->is_private_func);
    fileCIR->protos[this->Name] = proto_clone;
}

void FunctionAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    auto proto = std::make_unique<CIR::FunctionProto>(Proto->Name, Proto->cpoint_type, Proto->Args, Proto->is_variable_number_args, Proto->is_private_func);
    auto proto_clone = CIR::FunctionProto(Proto->Name, Proto->cpoint_type, Proto->Args, Proto->is_variable_number_args, Proto->is_private_func);
    fileCIR->protos[Proto->Name] = proto_clone;
    auto function = std::make_unique<CIR::Function>(std::move(proto));
    fileCIR->add_function(std::move(function));
    CIR::BasicBlock* entry_bb = fileCIR->add_basic_block(std::make_unique<CIR::BasicBlock>(fileCIR.get(), "entry"));
    fileCIR->set_insert_point(entry_bb);
    fileCIR->CurrentFunction->vars.clear();
    for (int i = 0; i < Proto->Args.size(); i++){
        auto init_arg_instr = std::make_unique<CIR::InitArgInstruction>(Proto->Args.at(i).first, Proto->Args.at(i).second);
        init_arg_instr->label = Proto->Args.at(i).first + ".init_arg";
        init_arg_instr->type = Proto->Args.at(i).second;
        init_arg_instr->type.is_ptr = true;
        init_arg_instr->type.nb_ptr++;
        auto init_arg = fileCIR->add_instruction(std::move(init_arg_instr));
        fileCIR->CurrentFunction->vars[Proto->Args.at(i).first] = CIR::Var(init_arg, Proto->Args.at(i).second);
    }
    createScope();
    CIR::InstructionRef ret_val;
    for (int i = 0; i < Body.size(); i++){
        ret_val = Body.at(i)->cir_gen(fileCIR);
    }

    endScope(fileCIR);

    bool body_contains_unreachable = false;
    for (int i = 0; i < Body.size(); i++){
        if (Body.at(i)->contains_expr(ExprType::Unreachable)){
            body_contains_unreachable = true;
        }
    }
    if (Proto->cpoint_type.type == void_type && !Proto->cpoint_type.is_ptr){
        auto void_instr = std::make_unique<CIR::ConstVoid>();
        ret_val = fileCIR->add_instruction(std::move(void_instr));
    } else if (Proto->Name == "main" && !body_contains_unreachable) {
        CIR::ConstNumber::nb_val_ty nb_val;
        nb_val.int_nb = 0;
        Cpoint_Type main_ret_type = Cpoint_Type(i32_type);
        auto main_ret_instr = std::make_unique<CIR::ConstNumber>(main_ret_type, false, nb_val);
        ret_val = fileCIR->add_instruction(std::move(main_ret_instr));
    }
    if (ret_val.get_type(fileCIR).type == never_type){
        fileCIR->add_instruction(std::make_unique<CIR::ConstNever>());
    } else if (!body_contains_unreachable){
        Cpoint_Type ret_val_type = fileCIR->get_instruction(ret_val.get_pos())->type;
        if (ret_val_type != Proto->cpoint_type){
            ret_val = fileCIR->add_instruction(std::make_unique<CIR::CastInstruction>(ret_val, ret_val_type, Proto->cpoint_type));
        }
        auto return_instr = std::make_unique<CIR::ReturnInstruction>(ret_val);
        return_instr->type = Cpoint_Type(never_type);
        fileCIR->add_instruction(std::move(return_instr));
    }
}

void StructDeclarAST::cir_gen(std::unique_ptr<FileCIR>& fileCIR){
    std::vector<std::pair<std::string, Cpoint_Type>> vars;
    for (int i = 0; i < Vars.size(); i++){
        // TODO : add support for support of multiple variables in one var
        vars.push_back(std::make_pair(Vars.at(i)->VarNames.at(0).first, Vars.at(i)->cpoint_type));
    }
    fileCIR->structs[Name] = CIR::Struct(Name, vars);
}

// remove parts of cir that would create problems with backends (ex : things generated after a never type with llvm)
void fixFileCIR(std::unique_ptr<FileCIR>& fileCIR){
    for (int function_nb = 0; function_nb < fileCIR->functions.size(); function_nb++){
        for (int bb_nb = 0; bb_nb < fileCIR->functions.at(function_nb)->basicBlocks.size(); bb_nb++){
            std::unique_ptr<CIR::BasicBlock>& bb = fileCIR->functions.at(function_nb)->basicBlocks.at(bb_nb);
            for (int instruction_nb = 0; instruction_nb < bb->instructions.size(); instruction_nb++){
                std::unique_ptr<CIR::Instruction>& instruction = bb->instructions.at(instruction_nb);
                if (instruction->type == never_type){
                    bb->instructions.erase(bb->instructions.begin()+instruction_nb+1, bb->instructions.end());
                }
            }
        }
    }
}

// do asserts on CIR
bool checkFileCIR(std::unique_ptr<FileCIR>& fileCIR){
    for (int function_nb = 0; function_nb < fileCIR->functions.size(); function_nb++){
        for (int bb_nb = 0; bb_nb < fileCIR->functions.at(function_nb)->basicBlocks.size(); bb_nb++){
            std::unique_ptr<CIR::Instruction>& last_instruction = fileCIR->functions.at(function_nb)->basicBlocks.at(bb_nb)->instructions.back();
            assert(last_instruction->type.type == never_type && "The last instruction of the CIR Basic Block should be never type");
        }
    }
    return true;
}

static void register_mod_functions_recursive(std::unique_ptr<FileCIR>& fileCIR, std::unique_ptr<ModAST>& mod, std::optional<std::string> parent_mods_prefix){
    for (int i = 0; i < mod->functions.size(); i++){
        std::string function_name = mod->functions.at(i)->Proto->Name;
        function_name = module_mangling(mod->mod_name, function_name);
        if (parent_mods_prefix != std::nullopt){
            //function_name = parent_mods_prefix.value() + function_name;
            function_name = module_mangling(parent_mods_prefix.value(), function_name);
        }
        auto Proto = mod->functions.at(i)->Proto->clone();
        Proto->Name = function_name;
        Proto->cir_gen(fileCIR);
        //FunctionProtos[function_name] = mod->functions.at(i)->Proto->clone();
    }
    for (int i = 0; i < mod->function_protos.size(); i++){
        std::string function_name = mod->function_protos.at(i)->Name;
        function_name = module_mangling(mod->mod_name, function_name);
        if (parent_mods_prefix != std::nullopt){
            //function_name = parent_mods_prefix.value() + function_name;
            function_name = module_mangling(parent_mods_prefix.value(), function_name);
        }
        auto Proto = mod->function_protos.at(i)->clone();
        Proto->Name = function_name;
        Proto->cir_gen(fileCIR);
    }

    for (int i = 0; i < mod->mods.size(); i++){
        std::string mod_prefix = mod->mod_name;
        if (parent_mods_prefix.has_value()){
            mod_prefix = module_mangling(parent_mods_prefix.value(), mod->mod_name);
        }
        register_mod_functions_recursive(fileCIR, mod->mods.at(i), std::optional(mod_prefix));
    }
}

static void register_mod_functions(std::unique_ptr<FileCIR>& fileCIR, std::unique_ptr<ModAST>& mod){
    register_mod_functions_recursive(fileCIR, mod, std::nullopt);
}

std::unique_ptr<FileCIR> FileAST::cir_gen(std::string filename){
  ZoneScopedN("CIR generation");
  auto fileCIR = std::make_unique<FileCIR>(filename, std::vector<std::unique_ptr<CIR::Function>>());
  add_externs_for_gc(fileCIR);
  for (int i = 0; i < mods.size(); i++){
    register_mod_functions(fileCIR, mods.at(i));
  }
  fileCIR->start_global_context();
  for (int i = 0; i < global_vars.size(); i++){
    global_vars.at(i)->cir_gen(fileCIR);
  }
  fileCIR->end_global_context();
  for (int i = 0; i < function_protos.size(); i++){
    function_protos.at(i)->cir_gen(fileCIR);
  }
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

  fixFileCIR(fileCIR);
  return fileCIR;
}

#endif