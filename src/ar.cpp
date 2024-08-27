#include "ar.h"
#include "llvm/Support/LLVMDriver.h"

extern int llvm_ar_main(int argc, char **argv, const llvm::ToolContext &);

int launch_ar(int argc, char** argv){
    return llvm_ar_main(argc, argv, {argv[0], nullptr, false});
}