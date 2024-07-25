#include "llvm/TargetParser/Triple.h"

int generate_llvm_object_file(std::string object_filename, llvm::Triple TripleLLVM, std::string LLVMTargetTriple, llvm::raw_ostream* file_out_ostream, bool PICmode, bool asm_mode, bool time_report, bool is_optimised, bool thread_sanitizer, int optimize_level, std::string CPU, std::string cpu_features);