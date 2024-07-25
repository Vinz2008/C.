#include "targets.h"

struct TargetInfo loongarch64_unknown_none_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "", // default target triple
        .pointer_size = 64,
        .features = "+f,+d",
    };
}