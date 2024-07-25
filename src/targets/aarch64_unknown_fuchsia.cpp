#include "targets.h"

TargetInfo aarch64_unknown_fuchsia_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "",
        .pointer_size = 64,
        .cpu = "",
        .features = "+v8a",
    };
}