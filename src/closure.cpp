#include "closure.h"
#include "llvm/TargetParser/Triple.h"
#include "types.h"
#include "codegen.h"
#include "debuginfo.h"
#include "targets/targets.h"
#include "abi.h"

extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<Module> TheModule;
extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unordered_map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;
extern bool debug_info_mode;

static int closure_number = 0;

static std::vector<std::unique_ptr<FunctionAST>> closuresToGenerate;

StructType* getClosureCapturedVarsStructType(std::vector<std::string> captured_vars){
    std::vector<Type*> structElements;
    for (int i = 0; i < captured_vars.size(); i++){
        Cpoint_Type* temp_type = get_variable_type(captured_vars.at(i));
        assert(temp_type != nullptr);
        structElements.push_back(get_type_llvm(*temp_type));
    }
    auto structType = StructType::get(*TheContext, structElements);
    std::string struct_name = "closure_struct" + std::to_string(closure_number);
    structType->setName(struct_name);
    DIType* structDebugInfosType = nullptr;
    std::vector<std::string> functions;
    std::vector<std::pair<std::string,Cpoint_Type>> captured_vars_with_type;
    for (int i = 0; i < captured_vars.size(); i++){
        captured_vars_with_type.push_back(std::make_pair(captured_vars.at(i), *get_variable_type(captured_vars.at(i))));
    }

    if (debug_info_mode){
        Cpoint_Type struct_type = Cpoint_Type(other_type, false, 0, false, 0, true, struct_name);
        structDebugInfosType = DebugInfoCreateStructType(struct_type, captured_vars_with_type, 0); // TODO : get LineNo/Pass LineNo to this function for debuginfos
    }
    StructDeclarations["closure_struct" + std::to_string(closure_number)] = std::make_unique<StructDeclaration>(dyn_cast<Type>(structType), structDebugInfosType, captured_vars_with_type, functions);
    return structType;
}

Value* getClosureCapturedVarsStruct(std::vector<std::string> captured_vars, StructType* structType){
    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                 TheFunction->getEntryBlock().begin());
    auto structAlloca = TmpB.CreateAlloca(structType, 0, "struct_closure");
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));
    for (int i = 0; i < captured_vars.size(); i++){
        auto index = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, i, true));
        Log::Info() << "Index GEP closure : " << i << "\n";
        Cpoint_Type captured_var_type = *get_variable_type(captured_vars.at(i));
        Log::Info() << "captured_var_type : " << captured_var_type << "\n";
        auto ptr = Builder->CreateGEP(structType, structAlloca, {zero, index}, "get_struct", true);
        Value* varValue = Builder->CreateLoad(get_type_llvm(*get_variable_type(captured_vars.at(i))), get_var_allocation(captured_vars.at(i)));
        Builder->CreateStore(varValue, ptr);
    }
    return structAlloca;
}

void generateClosures(){
    if (!closuresToGenerate.empty()){
        for (int i = 0; i < closuresToGenerate.size(); i++){
            closuresToGenerate.at(i)->codegen();
        }
    }
}

extern TargetInfo targetInfos;

extern Triple TripleLLVM;

Value* ClosureAST::codegen(){
    std::string closure_name = "closure" + std::to_string(closure_number);
    closure_number++;
    auto Proto = std::make_unique<PrototypeAST>(this->loc, closure_name, ArgNames, return_type, false, 0U, false, false, "", true);
    StructType* structType = nullptr;
    if (!captured_vars.empty()){
        structType = getClosureCapturedVarsStructType(captured_vars);
        if (!structType){
            return nullptr;
        }
        Cpoint_Type StructTypeCpoint = get_cpoint_type_from_llvm(structType);
        StructTypeCpoint.is_ptr = true;
        StructTypeCpoint.nb_ptr++;
        Proto->Args.insert(Proto->Args.begin(), std::make_pair("closure", StructTypeCpoint));
        
    }
    auto functionAST = std::make_unique<FunctionAST>(Proto->clone(), std::move(Body));
    closuresToGenerate.push_back(std::move(functionAST));
    auto f = Proto->codegen();
    if (!captured_vars.empty()){
        f->arg_begin()->addAttr(Attribute::Nest);
        Value* closureArgs = getClosureCapturedVarsStruct(captured_vars, structType);
        Function *TheFunction = Builder->GetInsertBlock()->getParent();
        auto trampAlloca = CreateEntryBlockAlloca(TheFunction, "tramp", Cpoint_Type(i8_type, false, 0, true, 72));
        // trampoline of 72 bytes  should be enough for all platforms : https://stackoverflow.com/questions/15509341/how-much-space-for-a-llvm-trampoline
        // Is it needed ? (according to the gcc codebase, some 64 bit platforms like aarch64 needs an alignement of 64, sparc needs 128, and nvptx 256)
        int pointer_size = get_pointer_size();
        if (TripleLLVM.isNVPTX()){
            trampAlloca->setAlignment(Align::Constant<256/8>());
        } else if (TripleLLVM.isSPARC()){
            trampAlloca->setAlignment(Align::Constant<128/8>());
        } else if (pointer_size == 32){
            trampAlloca->setAlignment(Align::Constant<32/8>());
        } else {
            trampAlloca->setAlignment(Align::Constant<64/8>());
        }
        std::vector<Value*> ArgsInitTrampoline;
        ArgsInitTrampoline.push_back(trampAlloca);
        ArgsInitTrampoline.push_back(f);
        ArgsInitTrampoline.push_back(closureArgs);
        Function* init_trampoline_func = Intrinsic::getDeclaration(TheModule.get(), Intrinsic::init_trampoline);
        Builder->CreateCall(init_trampoline_func, ArgsInitTrampoline);
        std::vector<Value*> ArgsAdjustTrampoline;
        ArgsAdjustTrampoline.push_back(trampAlloca);
        Function* adjust_trampoline_func = Intrinsic::getDeclaration(TheModule.get(), Intrinsic::adjust_trampoline);
        Value* function_with_trampoline = Builder->CreateCall(adjust_trampoline_func, ArgsAdjustTrampoline, "trampoline_closure_adjust");
        return function_with_trampoline;
    }
    return f;
}