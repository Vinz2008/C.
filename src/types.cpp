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

Type* get_type_llvm(Cpoint_Type cpoint_type){
    Type* type;
    if (cpoint_type.is_struct){
        type = StructDeclarations[cpoint_type.struct_name]->struct_type;
    } else {
    switch (cpoint_type.type){
        default:
        case double_type:
            type = Type::getDoubleTy(*TheContext);
            break;
        case int_type:
            type = Type::getInt32Ty(*TheContext);
            break;
        case float_type:
            type = Type::getFloatTy(*TheContext);
            break;
        case i8_type:
           type = Type::getInt8Ty(*TheContext);
           break;
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
        type = llvm::ArrayType::get(type, cpoint_type.nb_element);
    }
    return type;   
}

Value* get_default_value(Cpoint_Type type){
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
    "i8",
    "void",
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
