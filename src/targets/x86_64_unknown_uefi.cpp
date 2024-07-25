#include "targets.h"

struct TargetInfo x86_64_unknown_uefi_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "x86_64-unknown-windows",
        .pointer_size = 64,
        .cpu = "",
        .features = "-mmx,-sse,+soft-float",
    };
}