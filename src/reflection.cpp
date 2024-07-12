#include "reflection.h"
#include "types.h"
#include "log.h"

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<IRBuilder<>> Builder;

Value* getTypeId(Value* valueLLVM){
    Type* valType = valueLLVM->getType();
    Cpoint_Type cpoint_type = get_cpoint_type_from_llvm(valType);
    return ConstantInt::get(*TheContext, APInt(32, (uint64_t)cpoint_type.type));
}


static Value* refletionInstrTypeId(std::vector<std::unique_ptr<ExprAST>> Args){
    if (Args.size() > 1){
        return LogErrorV(emptyLoc, "wrong number of arguments for reflection function");
    }
    return getTypeId(Args.at(0)->codegen());
}

static Value* refletionInstrGetStructName(std::vector<std::unique_ptr<ExprAST>> Args){
    if (Args.size() > 1){
        return LogErrorV(emptyLoc, "wrong number of arguments for reflection function");
    }
    Value* val = Args.at(0)->codegen();
    if (!val->getType()->isStructTy()){
        return LogErrorV(emptyLoc, "argument is not a struct in getmembername");
    }
    auto structType = static_cast<StructType*>(val->getType());
    std::string structName = (std::string)structType->getStructName();
    Log::Info() << "get struct name : " << structName << "\n";
    return Builder->CreateGlobalStringPtr(StringRef(structName.c_str()));
}

static Value* refletionInstrGetMemberNb(std::vector<std::unique_ptr<ExprAST>> Args){
    if (Args.size() > 1){
        return LogErrorV(emptyLoc, "wrong number of arguments for reflection function");
    }
    Value* val = Args.at(0)->codegen();
    if (!val->getType()->isStructTy()){
        return LogErrorV(emptyLoc, "argument is not a struct in getmembername");
    }
    auto structType = static_cast<StructType*>(val->getType());
    int nb_member = -1;
    nb_member = structType->getNumElements();
    Log::Info() << "getmembernbname : " << nb_member << "\n";
    //return ConstantFP::get(*TheContext, APFloat((double)nb_member));
    return ConstantInt::get(*TheContext, APInt(32, (uint64_t)nb_member));
}

Value* refletionInstruction(std::string instruction, std::vector<std::unique_ptr<ExprAST>> Args){
    if (instruction == "typeid"){
        return refletionInstrTypeId(std::move(Args));
    } else if (instruction == "getstructname"){
        return refletionInstrGetStructName(std::move(Args));
    } else if (instruction == "getmembernb"){
        return refletionInstrGetMemberNb(std::move(Args));
    }
    return LogErrorV(emptyLoc, "Unknown Reflection Instruction");
}