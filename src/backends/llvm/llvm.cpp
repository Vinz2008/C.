#include "../../config.h"

#if ENABLE_CIR

#include "llvm.h"
#include "llvm_instructions.h"

#include "../../llvm.h"
#include "../../CIR/cir.h"

//#include "llvm/IR/IRBuilder.h"
//#include "llvm/IR/LLVMContext.h"

using namespace llvm;

/*static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;*/


// TODO : remove the tripleLLVM and just pass the TargetInfos and then use the target triple to construct the llvm Triple
// TODO : add PICMode to Comp_Context and pass a ref to it
int codegenLLVM(std::unique_ptr<FileCIR> fileCIR, std::string object_filename, Triple tripleLLVM, bool PICmode, bool asm_mode, bool time_report, bool is_optimised, bool thread_sanitizer, int optimize_level, std::string CPU, std::string cpu_features){
    auto LLVM_Context = std::make_unique<LLVMContext>();
    auto TheModule = std::make_unique<Module>(fileCIR->filename, *LLVM_Context);
    auto Builder = std::make_unique<IRBuilder<>>(*LLVM_Context);
    std::unique_ptr<LLVM::Context> codegen_context = std::make_unique<LLVM::Context>(std::move(LLVM_Context), std::move(TheModule), std::move(Builder));

    LLVM::codegenFile(codegen_context, std::move(fileCIR));


    std::error_code EC;
    llvm::raw_ostream* file_out_ostream = new raw_fd_ostream("out.cir.ll", EC);
    int ret = generate_llvm_object_file(std::move(codegen_context->TheModule), object_filename + ".cir.o", tripleLLVM, tripleLLVM.getTriple(), file_out_ostream, PICmode, asm_mode, time_report, is_optimised, thread_sanitizer, optimize_level, CPU, cpu_features);
    if (ret == 1){
        return 1;
    }

    return 0;
}

#endif