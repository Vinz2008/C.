#include "../../config.h"

#if ENABLE_CIR

#include "structs.h"
#include "llvm.h"
#include "llvm/IR/DerivedTypes.h"

using namespace llvm;


namespace LLVM {

void codegenStruct(std::unique_ptr<LLVM::Context> &codegen_context, CIR::Struct& structs){
    StructType* structType = StructType::create(*codegen_context->TheContext, structs.name); 
    std::vector<Type*> dataTypes;
    for (int i = 0; i < structs.vars.size(); i++){
        Type* var_type = get_type_llvm(codegen_context, structs.vars.at(i).second);
        dataTypes.push_back(var_type);
    }
    structType->setBody(dataTypes);
    codegen_context->structDeclars[structs.name] = structType;
    return;
}

}

#endif