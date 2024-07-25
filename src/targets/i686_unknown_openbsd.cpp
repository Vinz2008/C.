#include "targets.h"

TargetInfo i686_unknown_openbsd_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "",
        .pointer_size = 32,
        .cpu = "pentium4",
        .features = "",
    };
}