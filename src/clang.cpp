#include "config.h"

#ifdef ENABLE_LLVM_TOOLS_EMBEDDED_COMPILER

#include "llvm/Support/LLVMDriver.h"

extern int clang_main(int Argc, char **Argv, const llvm::ToolContext &ToolContext);

int launch_clang(int argc, char** argv){
    return clang_main(argc, argv, {argv[0], nullptr, false});
}

#endif