#include "config.h"

#ifdef ENABLE_LLVM_TOOLS_EMBEDDED_COMPILER

#include "lld.h"
#include <vector>
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/ArrayRef.h"
#include "lld/Common/Driver.h"

using namespace llvm;

using namespace lld;

// stolen from zig

namespace lld {
    namespace coff {
        bool link(llvm::ArrayRef<const char *> args, llvm::raw_ostream &stdoutOS,
                llvm::raw_ostream &stderrOS, bool exitEarly, bool disableOutput);
    }
    namespace elf {
        bool link(llvm::ArrayRef<const char *> args, llvm::raw_ostream &stdoutOS,
                llvm::raw_ostream &stderrOS, bool exitEarly, bool disableOutput);
    }
    namespace wasm {
        bool link(llvm::ArrayRef<const char *> args, llvm::raw_ostream &stdoutOS,
                llvm::raw_ostream &stderrOS, bool exitEarly, bool disableOutput);
    }
    namespace mingw {
        bool link(llvm::ArrayRef<const char *> args, llvm::raw_ostream &stdoutOS,
                llvm::raw_ostream &stderrOS, bool exitEarly, bool disableOutput);
    }
    namespace macho {
        bool link(llvm::ArrayRef<const char *> args, llvm::raw_ostream &stdoutOS,
                llvm::raw_ostream &stderrOS, bool exitEarly, bool disableOutput);
    }
}
bool can_use_internal_lld(Triple TripleLLVM){
    return TripleLLVM.isOSBinFormatELF() || TripleLLVM.isOSBinFormatWasm() || TripleLLVM.isOSBinFormatCOFF() || TripleLLVM.isOSBinFormatMachO();
}

int LLDLink(Triple TripleLLVM, int argc, const char **argv, bool can_exit_early, bool disable_output){
    std::vector<const char *> args(argv, argv + argc);
    if (!can_use_internal_lld(TripleLLVM)){
        fprintf(stderr, "trying to use the internal lld when it is not supported for this target triplet : %s\n", TripleLLVM.getTriple().c_str());
        return 1;
    }
    std::string lld_exe = "";
    if (TripleLLVM.isOSBinFormatELF()){
        lld_exe = "ld.lld";
    } else if (TripleLLVM.isOSBinFormatWasm()){
        lld_exe = "wasm-ld";
    } else if (TripleLLVM.isOSBinFormatCOFF()){
        lld_exe = "lld-link";
    } else if (TripleLLVM.isOSBinFormatMachO()){
        lld_exe = "ld64.lld";
    } else {
        // unreachable
    }
    args.insert(args.begin(), lld_exe.c_str());
    Result ret = lldMain(args, llvm::outs(), llvm::errs(), LLD_ALL_DRIVERS);
    return ret.retCode;
}

#endif