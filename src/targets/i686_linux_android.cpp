#include "targets.h"

TargetInfo i686_linux_android_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "", // default target triple
        .pointer_size = 32,
        .cpu = "pentiumpro",
        .features = "+mmx,+sse,+sse2,+sse3,+ssse3",
    };
}