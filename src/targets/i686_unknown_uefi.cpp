#include "targets.h"

struct TargetInfo i686_unknown_uefi_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "i686-unknown-windows-gnu",
        .pointer_size = 32,
        .cpu = "",
        .features = "-mmx,-sse,+soft-float",
    };
}