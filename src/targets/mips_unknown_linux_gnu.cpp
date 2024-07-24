#include "targets.h"

struct TargetInfo mips_unknown_linux_gnu_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "",
        .features = "+mips32r2,+fpxx,+nooddspreg",
    };
}