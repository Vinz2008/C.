#include "llvm/TargetParser/Triple.h"

int generate_llvm_object_file(std::string object_filename, llvm::Triple TripleLLVM, std::string TargetTriple, llvm::raw_ostream* file_out_ostream, bool PICmode, bool asm_mode, bool time_report, bool is_optimised, bool thread_sanitizer, int optimize_level);