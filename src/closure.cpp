#include "closure.h"
#include "types.h"
#include "codegen.h"

extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<Module> TheModule;
extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unordered_map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;

static int closure_number = 0;

static std::vector<std::unique_ptr<FunctionAST>> closuresToGenerate;

StructType* getClosureCapturedVarsStructType(std::vector<std::string> captured_vars){
    std::vector<Type*> structElements;
    for (int i = 0; i < captured_vars.size(); i++){
        Cpoint_Type* temp_type = get_variable_type(captured_vars.at(i));
        if (!temp_type){
            LogErrorV(emptyLoc, "Variable captured by closure doesn't exist");
            return nullptr;
        }
        structElements.push_back(get_type_llvm(*temp_type));
    }
    auto structType = StructType::get(*TheContext, structElements);
    structType->setName("closure_struct" + std::to_string(closure_number)); // TODO : remove this ?
    std::vector<std::string> functions;
    std::vector<std::pair<std::string,Cpoint_Type>> captured_vars_with_type;
    for (int i = 0; i < captured_vars.size(); i++){
        captured_vars_with_type.push_back(std::make_pair(captured_vars.at(i), *get_variable_type(captured_vars.at(i))));
    }
    StructDeclarations["closure_struct" + std::to_string(closure_number)] = std::make_unique<StructDeclaration>(dyn_cast<Type>(structType), captured_vars_with_type, functions);
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
        Proto->Args.insert(Proto->Args.begin(), std::make_pair("closure", get_cpoint_type_from_llvm(structType)));
        
    }
    auto functionAST = std::make_unique<FunctionAST>(Proto->clone(), std::move(Body));
    closuresToGenerate.push_back(std::move(functionAST));
    auto f = Proto->codegen();
    if (!captured_vars.empty()){
        Value* closureArgs = getClosureCapturedVarsStruct(captured_vars, structType);
        Function *TheFunction = Builder->GetInsertBlock()->getParent();
        auto trampAlloca = CreateEntryBlockAlloca(TheFunction, "tramp", Cpoint_Type(i8_type, false, 0, true, 72));
        trampAlloca->setAlignment(Align::Constant<16>()); // Is it needed ?
        // trampoline of 72 bytes and alignement of 16 should be enough for all platforms : https://stackoverflow.com/questions/15509341/how-much-space-for-a-llvm-trampoline
        std::vector<Value*> ArgsInitTrampoline;
        ArgsInitTrampoline.push_back(trampAlloca);
        ArgsInitTrampoline.push_back(f);
        ArgsInitTrampoline.push_back(closureArgs);
        Function* init_trampoline_func = Intrinsic::getDeclaration(TheModule.get(), Intrinsic::init_trampoline);
        Builder->CreateCall(init_trampoline_func, ArgsInitTrampoline);
        std::vector<Value*> ArgsAdjustTrampoline;
        ArgsAdjustTrampoline.push_back(trampAlloca);
        Function* adjust_trampoline_func = Intrinsic::getDeclaration(TheModule.get(), Intrinsic::adjust_trampoline);
        auto function_with_trampoline = Builder->CreateCall(adjust_trampoline_func, ArgsAdjustTrampoline, "trampoline_closure_adjust");
        return function_with_trampoline;
    }
    return f;
}