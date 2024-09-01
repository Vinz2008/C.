#include "backends.h"
#include "../CIR/cir.h"
#include "./llvm/llvm.h"
#include <cstdio>
#include <cstdlib>

int codegenBackend(std::unique_ptr<FileCIR> fileCIR, enum backend_type backend, std::string object_filename, Triple tripleLLVM, bool PICmode, bool asm_mode, bool time_report, bool is_optimised, bool thread_sanitizer, int optimize_level, std::string CPU, std::string cpu_features){
    if (backend == backend_type::LLVM_TYPE){
        return codegenLLVM(std::move(fileCIR), object_filename, tripleLLVM, PICmode, asm_mode, time_report, is_optimised, thread_sanitizer, optimize_level, CPU, cpu_features);
    } else {
        fprintf(stderr, "Unknown backend");
        exit(1);
    }

}