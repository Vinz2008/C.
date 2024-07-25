#include "targets.h"

TargetInfo aarch64_unknown_uefi_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "aarch64-unknown-window",
        .pointer_size = 64,
        .cpu = "",
        .features = "+v8a",
    };
}