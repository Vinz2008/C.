#include "types.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;

Type* get_type(int t){
    switch (t){
        case double_type:
            return Type::getDoubleTy(*TheContext);
        case int_type:
            return Type::getInt32Ty(*TheContext);
        case float_type:
            return Type::getFloatTy(*TheContext);
    }
}
