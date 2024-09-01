#include <memory>

class FileCIR;

namespace LLVM {
    class Context;
}

namespace LLVM {
    void codegenFile(std::unique_ptr<LLVM::Context>& codegen_context, std::unique_ptr<FileCIR> fileCIR);
}