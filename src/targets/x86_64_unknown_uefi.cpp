#include "targets.h"

struct TargetInfo x86_64_unknown_uefi_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "x86_64-unknown-windows",
        .pointer_size = 64,
        // As documented in https://developer.android.com/ndk/guides/cpu-features.html
        // the neon (ASIMD) and FP must exist on all android aarch64 targets.
        .features = "-mmx,-sse,+soft-float",
    };
}