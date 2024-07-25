#include "targets.h"

TargetInfo loongarch64_unknown_linux_gnu_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "", // default target triple
        .pointer_size = 64,
        .cpu = "",
        .features = "+f,+d",
    };
}