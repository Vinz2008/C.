#include "reflection.h"
#include "types.h"
#include "log.h"
#include "ast.h"


#include "CIR/cir.h"

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<IRBuilder<>> Builder;

/*Value* getTypeId(Value* valueLLVM){
    Type* valType = valueLLVM->getType();
    Cpoint_Type cpoint_type = get_cpoint_type_from_llvm(valType);
    return ConstantInt::get(*TheContext, APInt(32, (uint64_t)cpoint_type.type));
}*/

int getTypeId(Cpoint_Type cpoint_type){
    return cpoint_type.type;
}

Value* getTypeIdLLVM(Cpoint_Type cpoint_type){
    return ConstantInt::get(*TheContext, APInt(32, getTypeId(cpoint_type)));
}

static Value* refletionInstrTypeId(std::vector<std::unique_ptr<ExprAST>> Args){
    if (Args.size() > 1){
        return LogErrorV(emptyLoc, "wrong number of arguments for reflection function");
    }
    return getTypeIdLLVM(Args.at(0)->get_type());
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


#if ENABLE_CIR

CIR::InstructionRef getTypeIdCIR(std::unique_ptr<FileCIR>& fileCIR, Cpoint_Type cpoint_type){
    CIR::ConstNumber::nb_val_ty nb_val;
    nb_val.int_nb = getTypeId(cpoint_type);
    return fileCIR->add_instruction(std::make_unique<CIR::ConstNumber>(Cpoint_Type(i32_type), false, nb_val));
}



static CIR::InstructionRef refletionInstrTypeIdCIR(std::unique_ptr<FileCIR>& fileCIR, std::vector<std::unique_ptr<ExprAST>> Args){
    if (Args.size() > 1){
        LogError("wrong number of arguments for reflection function");
        return CIR::InstructionRef();
    }
    return getTypeIdCIR(fileCIR, Args.at(0)->get_type());
}


static CIR::InstructionRef refletionInstrGetStructNameCIR(std::unique_ptr<FileCIR>& fileCIR, std::vector<std::unique_ptr<ExprAST>> Args){
    if (Args.size() > 1){
        LogError("wrong number of arguments for reflection function");
        return CIR::InstructionRef();
    }
    Cpoint_Type val_type = Args.at(0)->get_type(fileCIR.get());
    if (!val_type.is_struct){
        LogError("argument is not a struct in getmembername");
        return CIR::InstructionRef();
    }
    std::string structName = val_type.struct_name;
    Log::Info() << "get struct name : " << structName << "\n";
    return fileCIR->add_string(structName);
}

static CIR::InstructionRef refletionInstrGetMemberNbCIR(std::unique_ptr<FileCIR>& fileCIR, std::vector<std::unique_ptr<ExprAST>> Args){
    if (Args.size() > 1){
        LogError("wrong number of arguments for reflection function");
        return CIR::InstructionRef();
    }
    Cpoint_Type val_type = Args.at(0)->get_type(fileCIR.get());
    if (!val_type.is_struct){
        LogError("argument is not a struct in getmembername");
        return CIR::InstructionRef();
    }
    int nb_member = fileCIR->structs[val_type.struct_name].vars.size();
    Log::Info() << "getmembernbname : " << nb_member << "\n";
    CIR::ConstNumber::nb_val_ty nb_val;
    nb_val.int_nb = nb_member;
    return fileCIR->add_instruction(std::make_unique<CIR::ConstNumber>(Cpoint_Type(i32_type), false, nb_val));
}


CIR::InstructionRef refletionInstructionCIR(std::unique_ptr<FileCIR>& fileCIR, std::string instruction, std::vector<std::unique_ptr<ExprAST>> Args){
    if (instruction == "typeid"){
        return refletionInstrTypeIdCIR(fileCIR, std::move(Args));
    } else if (instruction == "getstructname"){
        return refletionInstrGetStructNameCIR(fileCIR, std::move(Args));
    } else if (instruction == "getmembernb"){
        return refletionInstrGetMemberNbCIR(fileCIR, std::move(Args));
    }
    LogError("Unknown Reflection Instruction");
    return CIR::InstructionRef();
}


#endif