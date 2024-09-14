#include "../../CIR/cir.h"

namespace LLVM {
    class Context;
    void codegenStruct(std::unique_ptr<LLVM::Context> &codegen_context, CIR::Struct& structs);
}