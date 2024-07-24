#include "targets.h"

struct TargetInfo loongarch64_unknown_linux_gnu_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "", // default target triple
        .features = "+f,+d",
    };
}