#include <memory>
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

class FileCIR;

enum backend_type {
    LLVM_TYPE,
};

int codegenBackend(std::unique_ptr<FileCIR> fileCIR, enum backend_type backend, std::string object_filename, Triple tripleLLVM, bool PICmode, bool asm_mode, bool time_report, bool is_optimised, bool thread_sanitizer, int optimize_level, std::string CPU, std::string cpu_features);