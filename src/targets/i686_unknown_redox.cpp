#include "targets.h"

TargetInfo i686_unknown_redox_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "",
        .pointer_size = 32,
        .cpu = "pentiumpro",
        .features = "",
    };
}