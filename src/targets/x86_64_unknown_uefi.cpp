#include "targets.h"

TargetInfo x86_64_unknown_uefi_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "x86_64-unknown-windows",
        .pointer_size = 64,
        .cpu = "x86-64",
        .features = "-mmx,-sse,+soft-float",
    };
}