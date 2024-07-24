#include "targets.h"

struct TargetInfo aarch64_unknown_none_softfloat_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "aarch64-unknown-none",
        .features = "+v8a,+strict-align,-neon,-fp-armv8",
    };
}