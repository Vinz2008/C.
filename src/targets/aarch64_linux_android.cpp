#include "targets.h"

TargetInfo aarch64_linux_android_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "", // default target triple
        .pointer_size = 64,
        .cpu = "",
        // As documented in https://developer.android.com/ndk/guides/cpu-features.html
        // the neon (ASIMD) and FP must exist on all android aarch64 targets.
        .features = "+v8a,+neon,+fp-armv8",
    };
}