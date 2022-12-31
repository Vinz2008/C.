#include "types.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;

Type* get_type_llvm(int t, bool is_ptr){
    switch (t){
        case double_type:
            if (is_ptr){
            return Type::getDoublePtrTy(*TheContext);
            } else {
            return Type::getDoubleTy(*TheContext);
            }
        case int_type:
        if (is_ptr){
            return Type::getInt32PtrTy(*TheContext);
        } else {
            return Type::getInt32Ty(*TheContext);
        }
        case float_type:
        if (is_ptr){
            return Type::getFloatPtrTy(*TheContext);
        } else {
            return Type::getFloatTy(*TheContext);
        }
    }
}

std::vector<std::string> types{
    "double",
    "int",
    "float",
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
