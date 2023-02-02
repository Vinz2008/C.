#include "llvm/IR/Instructions.h"
#include "types.h"

void InitializeModule(std::string filename);

class NamedValue {
public:
    llvm::AllocaInst* alloca_inst;
    Cpoint_Type type;
    NamedValue(llvm::AllocaInst* alloca_inst, Cpoint_Type type) : alloca_inst(alloca_inst), type(std::move(type)) {}
};
