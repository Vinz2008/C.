#include "types.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Value.h"
#include "codegen.h"
#include "ast.h"
#include "log.h"
#include <map>

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;
extern std::map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;
extern std::map<std::string, std::unique_ptr<ClassDeclaration>> ClassDeclarations;

Type* get_type_llvm(Cpoint_Type cpoint_type){
    Type* type;
    Log::Info() << "cpoint_type.is_struct : " << cpoint_type.is_struct << "\n";
    if (cpoint_type.is_struct){
        type = StructDeclarations[cpoint_type.struct_name]->struct_type;
    } else if (cpoint_type.is_class){
        Log::Info() << "cpoint_type.class_name : " << cpoint_type.class_name << "\n";
        if (ClassDeclarations[cpoint_type.class_name] == nullptr){
            Log::Info() << "ClassDeclarations[cpoint_type.class_name] NULLPTR" << "\n";
        }
        type = ClassDeclarations[cpoint_type.class_name]->class_type;
    } else {
    switch (cpoint_type.type){
        default:
        case double_type:
            type = Type::getDoubleTy(*TheContext);
            break;
        case i32_type:
        case int_type:
            type = Type::getInt32Ty(*TheContext);
            break;
        case float_type:
            type = Type::getFloatTy(*TheContext);
            break;
        case i8_type:
           type = Type::getInt8Ty(*TheContext);
           break;
        case i16_type:
            type = Type::getInt16Ty(*TheContext);
        case i64_type:
            type = Type::getInt64Ty(*TheContext);
        case void_type:
            if (!cpoint_type.is_ptr){
            type = Type::getVoidTy(*TheContext);
            } else {
            type = PointerType::get(*TheContext, 0U);
            if (cpoint_type.is_array){
                type = llvm::ArrayType::get(type, cpoint_type.nb_element);
            }
            }
            return type;
        case argv_type:
            return Type::getInt8PtrTy(*TheContext)->getPointerTo();
    }
    }
    if (cpoint_type.is_ptr){
        for (int i = 0; i < cpoint_type.nb_ptr; i++){
        type = type->getPointerTo();
        }
    }
    if (cpoint_type.is_array){
        Log::Info() << "create array type with " << cpoint_type.nb_element << " member of type of " << type << "\n";
        type = llvm::ArrayType::get(type, cpoint_type.nb_element);
    }
    return type;   
}

Cpoint_Type* get_cpoint_type_from_llvm(Type* llvm_type){
    int type = double_type;
    bool is_ptr = false;
    Type* not_ptr_type = llvm_type;
    if (llvm_type->isPointerTy()){
        is_ptr = true;
    }
    if (llvm_type == Type::getDoubleTy(*TheContext)){
        type = double_type;
    } else if (llvm_type == Type::getInt32Ty(*TheContext)){
        type = int_type;
    } else if (llvm_type == Type::getFloatTy(*TheContext)){
        type = float_type;
    } else if (llvm_type == Type::getInt8Ty(*TheContext)){
        type = i8_type;
    } else if (llvm_type == Type::getInt16Ty(*TheContext)){
        type = i16_type;
    } else if (llvm_type == Type::getInt64Ty(*TheContext)){
        type = i64_type;
    } else {
        if (is_ptr){
            type = i8_type;
        } else {
        Log::Warning() << "Unknown Type" << "\n";
        }
    }
    Cpoint_Type* cpoint_type = new Cpoint_Type(type, is_ptr);
    return cpoint_type;
}

Value* get_default_value(Cpoint_Type type){
    if (type.is_ptr){
        return ConstantPointerNull::get(PointerType::get(*TheContext, 0));
    }
    switch (type.type){
        default:
        case double_type:
            return ConstantFP::get(*TheContext, APFloat(0.0));
        case i32_type:
        case int_type:
            return ConstantInt::get(*TheContext, APInt(32, 0, true));
        case i8_type:
            return ConstantInt::get(*TheContext, APInt(8, 0, true));
        case i16_type:
            return ConstantInt::get(*TheContext, APInt(16, 0, true));
        case i64_type:
            return ConstantInt::get(*TheContext, APInt(64, 0, true));
    }
    return ConstantFP::get(*TheContext, APFloat(0.0));
}

Constant* get_default_constant(Cpoint_Type type){
    if (type.is_ptr){
        return ConstantPointerNull::get(PointerType::get(*TheContext, 0));
    }
    switch (type.type){
        default:
        case double_type:
            return ConstantFP::get(*TheContext, APFloat(0.0));
        case int_type:
            return ConstantInt::get(*TheContext, APInt(32, 0, true));
        case i8_type:
            return ConstantInt::get(*TheContext, APInt(8, 0, true));
    }
    return ConstantFP::get(*TheContext, APFloat(0.0));
}


std::vector<std::string> types{
    "double",
    "int",
    "float",
    "void",
    "i8",
    "i16",
    "i32",
    "i64",
};

bool is_type(std::string type){
    for (int i = 0; i < types.size(); i++){
       if (type.compare(types.at(i)) == 0){
	    return true;
       }
    }
    return false;
}

int get_type(std::string type){
    for (int i = 0; i < types.size(); i++){
       if (type.compare(types.at(i)) == 0){
        return -(i + 1);
       }
    }
    return 1;
}
