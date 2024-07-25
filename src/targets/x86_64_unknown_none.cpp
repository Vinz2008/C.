
#include "targets.h"

struct TargetInfo x86_64_unknown_none_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "x86_64-unknown-none-elf",
        .pointer_size = 64,
        .cpu = "",
        .features = "-mmx,-sse,-sse2,-sse3,-ssse3,-sse4.1,-sse4.2,-avx,-avx2,+soft-float",
    };
}