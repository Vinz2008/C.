#include "types.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;

Type* get_type_llvm(int t){
    switch (t){
        case double_type:
            return Type::getDoubleTy(*TheContext);
        case int_type:
            return Type::getInt32Ty(*TheContext);
        case float_type:
            return Type::getFloatTy(*TheContext);
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
