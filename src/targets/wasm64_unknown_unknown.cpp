#include "targets.h"

TargetInfo wasm64_unknown_unknown_get_target_infos(){
    return TargetInfo {
        .llvm_target_triple = "",
        .pointer_size = 64,
        .cpu = "",
        .features = "+bulk-memory,+mutable-globals,+sign-ext,+nontrapping-fptoint",
    };
}