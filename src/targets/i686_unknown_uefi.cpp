#include "targets.h"

struct TargetInfo i686_unknown_uefi_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "i686-unknown-windows-gnu",
        // As documented in https://developer.android.com/ndk/guides/cpu-features.html
        // the neon (ASIMD) and FP must exist on all android aarch64 targets.
        .features = "-mmx,-sse,+soft-float",
    };
}