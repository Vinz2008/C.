#include "targets.h"

TargetInfo mips_unknown_linux_gnu_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "",
        .pointer_size = 32,
        .cpu = "mips32r2",
        .features = "+mips32r2,+fpxx,+nooddspreg",
    };
}