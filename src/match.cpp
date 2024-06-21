#include "match.h"
#include "llvm/IR/IRBuilder.h"
#include "codegen.h"
#include "operators.h"

extern std::unordered_map<std::string, std::unique_ptr<NamedValue>> NamedValues;

extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unordered_map<std::string, std::unique_ptr<UnionDeclaration>> UnionDeclarations;
extern std::unordered_map<std::string, std::unique_ptr<EnumDeclaration>> EnumDeclarations;

static void string_vector_erase(std::vector<std::string>& strings, std::string string){
    std::vector<std::string>::iterator iter = strings.begin();
    while (iter != strings.end()){
        if (*iter == string){
            iter = strings.erase(iter);
        } else {
           ++iter;
        }
    }
}


Value* MatchNotEnumCodegen(std::string matchVar, std::vector<std::unique_ptr<matchCase>> matchCases, Function* TheFunction){
    // For now consider by default it will compare ints
    AllocaInst* Alloca = NamedValues[matchVar]->alloca_inst;
    Cpoint_Type var_type = NamedValues[matchVar]->type;
    Value* val_from_var = Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, "load_match_const");
    bool is_bool = false;
    Log::Info() << "MatchNotEnumCodegen Alloca->getAllocatedType() : " << get_cpoint_type_from_llvm(Alloca->getAllocatedType()) << "\n";
    if (Alloca->getAllocatedType() == Type::getInt1Ty(*TheContext)){
        is_bool = true;
    }
    if (is_bool){
        val_from_var = Builder->CreateZExt(val_from_var, get_type_llvm(Cpoint_Type(i32_type)), "cast_bool_match");
    } else if (Alloca->getAllocatedType() != get_type_llvm(Cpoint_Type(i32_type))){
        //convert_to_type(get_cpoint_type_from_llvm(val_from_var->getType()), get_type_llvm(Cpoint_Type(i32_type)), val_from_var);
        convert_to_type(var_type, Cpoint_Type(i32_type), val_from_var);
    }

    // for testing
    //std::vector<Value*> Args;
    //Args.push_back(val_from_var);
    //Builder->CreateCall(getFunction("printi"), Args);

    auto switch_inst = Builder->CreateSwitch(val_from_var, nullptr, matchCases.size());
    BasicBlock* defaultDestBB;
    BasicBlock* AfterBB = BasicBlock::Create(*TheContext, "After_match");
    bool found_underscore = false;
    for (int i = 0; i < matchCases.size(); i++){
        std::unique_ptr<matchCase> matchCaseTemp = matchCases.at(i)->clone();
        if (matchCaseTemp->is_underscore){
            found_underscore = true;
            defaultDestBB = BasicBlock::Create(*TheContext, "default_dest", TheFunction);
            Builder->SetInsertPoint(defaultDestBB);
            matchCaseTemp->Body->codegen();
            //for (int j = 0; j < matchCaseTemp->Body.size(); j++){
            //    matchCaseTemp->Body.at(j)->codegen();
            //}
            Builder->CreateBr(AfterBB);
            switch_inst->setDefaultDest(defaultDestBB);
        } else {
            BasicBlock* thenBB = BasicBlock::Create(*TheContext, "then_match_const", TheFunction);
            Builder->SetInsertPoint(thenBB);
            auto valExpr = matchCaseTemp->expr->clone(); 
            Value* val = nullptr;
            if (dynamic_cast<NumberExprAST*>(valExpr.get())){
                val = valExpr->codegen();
            } else if (dynamic_cast<UnaryExprAST*>(valExpr.get())){
                Log::Info() << "TEST UNARY" << "\n";
                auto unExpr = get_Expr_from_ExprAST<UnaryExprAST>(std::move(valExpr));
                if (unExpr->Opcode == '-'){
                    if (dynamic_cast<NumberExprAST*>(unExpr->Operand.get())){
                        auto exprFromUnary = get_Expr_from_ExprAST<NumberExprAST>(std::move(unExpr->Operand));
                        exprFromUnary->Val = -exprFromUnary->Val;
                        Log::Info() << "exprFromUnary->Val : " << exprFromUnary->Val << "\n";
                        //unExpr->Operand = std::move(exprFromUnary);
                        //val = exprFromUnary->codegen();
                        //val = ConstantFP::get(*TheContext, APFloat(exprFromUnary->Val));
                        val = ConstantInt::get(*TheContext, APInt(32, (uint64_t)exprFromUnary->Val));
                        if (!val){
                            return LogErrorV(emptyLoc, "In the match value, couldn't generate the value after the '-'");
                        }
                    } else {
                        return LogErrorV(emptyLoc, "In the match value, there is no constant number after the '-'");
                    }
                } else {
                    return LogErrorV(emptyLoc, "In the match value, using an unary opertor that is not '-'");
                    //val = unExpr->codegen();
                }
            } else {
                val = valExpr->codegen();
                //return LogErrorV(emptyLoc, "Expression not supported in match case value");
            }
            Log::Info() << "match val : " << get_cpoint_type_from_llvm(val->getType()) << "\n";
            if (val->getType() != get_type_llvm(i32_type)){
                convert_to_type(get_cpoint_type_from_llvm(val->getType()), get_type_llvm(i32_type), val);
            }
            Log::Info() << "match val after converting : " << get_cpoint_type_from_llvm(val->getType()) << "\n";
            matchCaseTemp->Body->codegen();
            //for (int j = 0; j < matchCaseTemp->Body.size(); j++){
            //    matchCaseTemp->Body.at(j)->codegen();
            //}
            Builder->CreateBr(AfterBB);
            ConstantInt* constint_val = nullptr;
            Log::Info() << "value id : " << val->getValueID() << "\n";
            if (dyn_cast<ConstantInt>(val)){
                constint_val = dyn_cast<ConstantInt>(val);
            } else {
                //return LogErrorV(emptyLoc, "The match value is not a constant int");
            }
            switch_inst->addCase(constint_val, thenBB);
        }
    }
    if (is_bool && !found_underscore){
        defaultDestBB = BasicBlock::Create(*TheContext, "default_dest_unreachable", TheFunction);
        Builder->SetInsertPoint(defaultDestBB);
        // for testing
        //std::vector<std::unique_ptr<ExprAST>> Args;
        //Args.push_back(std::make_unique<StringExprAST>("not accessible"));
        //CallExprAST(emptyLoc, "printstr", std::move(Args), Cpoint_Type(double_type)).codegen();
        Builder->CreateBr(AfterBB);
        switch_inst->setDefaultDest(defaultDestBB);
    }
    if (!found_underscore && !is_bool){
        return LogErrorV(emptyLoc, "Not all cases were verified in the int match case");
    }
    TheFunction->insert(TheFunction->end(), AfterBB);
    Builder->SetInsertPoint(AfterBB);

    return Constant::getNullValue(get_type_llvm(double_type));
}

Value* MatchExprAST::codegen(){
    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    Value* tag;
    Value* tag_ptr;
    Value* val_ptr = nullptr;
    std::unique_ptr<EnumDeclarAST> enumDeclar;
    if (NamedValues[matchVar] == nullptr){
        return LogErrorV(this->loc, "Match var is unknown or the match expression is invalid");
    }
    if (!NamedValues[matchVar]->type.is_enum){
        //is_enum = false;
        return MatchNotEnumCodegen(matchVar, std::move(matchCases), TheFunction);
    }
    enumDeclar = EnumDeclarations[NamedValues[matchVar]->type.enum_name]->EnumDeclar->clone();
    bool enum_member_contain_type = enumDeclar->enum_member_contain_type;
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    auto index_tag = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));
    auto index_val = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
    
    if (!enum_member_contain_type){
        tag = Builder->CreateLoad(get_type_llvm(i32_type), NamedValues[matchVar]->alloca_inst, matchVar);
    } else {
        tag_ptr = Builder->CreateGEP(get_type_llvm(NamedValues[matchVar]->type), NamedValues[matchVar]->alloca_inst, { zero, index_tag }, "tag_ptr", true);
        val_ptr = Builder->CreateGEP(get_type_llvm(NamedValues[matchVar]->type), NamedValues[matchVar]->alloca_inst, { zero, index_val }, "val_ptr", true);
        tag = Builder->CreateLoad(get_type_llvm(i32_type), tag_ptr, matchVar);
    }
    BasicBlock *AfterMatch = BasicBlock::Create(*TheContext, "after_match");
    std::vector<std::string> membersNotFound;
    for (int i = 0; i < enumDeclar->EnumMembers.size(); i++){
        membersNotFound.push_back(enumDeclar->EnumMembers.at(i)->Name);
    }
    for (int i = 0; i < matchCases.size(); i++){
        std::unique_ptr<matchCase> matchCaseTemp = matchCases.at(i)->clone();
        BasicBlock *ElseBB;
        if (matchCaseTemp->is_underscore){
            //Builder->CreateBr(AfterMatch);
            matchCaseTemp->Body->codegen();
            // for (int i = 0; i < matchCaseTemp->Body.size(); i++){
            //     matchCaseTemp->Body.at(i)->codegen();
            // }
            membersNotFound.clear();
            break;
        } else {
        string_vector_erase(membersNotFound, matchCaseTemp->enum_member);
        if (matchCaseTemp->enum_name != NamedValues[matchVar]->type.enum_name){
            return LogErrorV(this->loc, "The match case is using a member of a different enum than the one in the expression");
        }
        int pos = -1;
        bool has_custom_index = false;
        for (int j = 0; j < enumDeclar->EnumMembers.size(); j++){
            if (enumDeclar->EnumMembers[j]->Name == matchCaseTemp->enum_member){
                if (enumDeclar->EnumMembers[j]->contains_custom_index){
                    has_custom_index = true;
                    pos = enumDeclar->EnumMembers[j]->Index;
                } else {
                    pos = j;
                }
            }
        }
        if (pos == -1 && !has_custom_index){
            return LogErrorV(this->loc, "Couldn't find the member of this enum in match case");
        }
        Value* cmp = operators::LLVMCreateCmp(tag, ConstantInt::get(get_type_llvm(i32_type), APInt(32, (uint64_t)pos))); // TODO : replace cmp with switch
        //cmp = Builder->CreateFCmpONE(cmp, ConstantFP::get(*TheContext, APFloat(0.0)), "ifcond");
        BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "then_match", TheFunction);
        ElseBB = BasicBlock::Create(*TheContext, "else_match");
        Builder->CreateCondBr(cmp, ThenBB, ElseBB);
        Builder->SetInsertPoint(ThenBB);
        if (matchCaseTemp->var_name != ""){
            if (!enumDeclar->EnumMembers[pos]->contains_value){
                return LogErrorV(this->loc, "Enum Member doesn't contain a value");
            }
            AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, matchCaseTemp->var_name, *enumDeclar->EnumMembers[pos]->Type);
            Value* enum_val = Builder->CreateLoad(get_type_llvm(*enumDeclar->EnumMembers[pos]->Type), val_ptr, "enum_val_load");
            Builder->CreateStore(enum_val, Alloca);
            NamedValues[matchCaseTemp->var_name] = std::make_unique<NamedValue>(Alloca, *enumDeclar->EnumMembers[pos]->Type);
            Log::Info() << "Create var for match : " << enumDeclar->EnumMembers[pos]->Name << "\n";
        }
        }
        matchCaseTemp->Body->codegen();
        // for (int i = 0; i < matchCaseTemp->Body.size(); i++){
        //     matchCaseTemp->Body.at(i)->codegen();
        // }
        Builder->CreateBr(AfterMatch);
        //Builder->CreateBr(ElseBB);
        //TheFunction->getBasicBlockList().push_back(ElseBB);
        TheFunction->insert(TheFunction->end(), ElseBB);
        Builder->SetInsertPoint(ElseBB);
    }
    if (!membersNotFound.empty()){
        std::string list_members_not_found_str = "";
        int i;
        for (i = 0; i < membersNotFound.size()-1; i++){
            list_members_not_found_str += membersNotFound.at(i) + ", ";
        }
        list_members_not_found_str += membersNotFound.at(i);
        return LogErrorV(this->loc, "These members were not found in the match case : %s\n", list_members_not_found_str.c_str());
    }
    Builder->CreateBr(AfterMatch);
    //TheFunction->getBasicBlockList().push_back(AfterMatch);
    TheFunction->insert(TheFunction->end(), AfterMatch);
    Builder->SetInsertPoint(AfterMatch);
    return Constant::getNullValue(get_type_llvm(double_type));
}