#include "targets.h"

struct TargetInfo x86_64_pc_windows_gnu_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "",
        .pointer_size = 64,
        .features = "+cx16,+sse3,+sahf",
    };
}