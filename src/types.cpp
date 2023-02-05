#include "types.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "codegen.h"
#include "ast.h"
#include <map>

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;
extern std::map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;

Type* get_type_llvm(int t, bool is_ptr, bool is_array, int nb_aray_elements, bool is_struct, std::string struct_name){
    Type* type;
    if (is_struct){
        type = StructDeclarations[struct_name]->struct_type;
    } else {
    switch (t){
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
            if (!is_ptr){
            type = Type::getVoidTy(*TheContext);
            } else {
            type = PointerType::get(*TheContext, 0U);
            if (is_array){
                type = llvm::ArrayType::get(type, nb_aray_elements);
            }
            }
            return type;
        case argv_type:
            return Type::getInt8PtrTy(*TheContext)->getPointerTo();
    }
    }
    if (is_ptr){
        type = type->getPointerTo();
    }
    if (is_array){
        type = llvm::ArrayType::get(type, nb_aray_elements);
    }
    return type;
    
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
