#include "targets.h"

TargetInfo i686_unknown_linux_gnu_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "", // default target triple
        .pointer_size = 32,
        .cpu = "pentium4",
        .features = "",
    };
}