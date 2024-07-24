#include "targets.h"

struct TargetInfo i686_linux_android_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "", // default target triple
        .features = "+mmx,+sse,+sse2,+sse3,+ssse3",
    };
}