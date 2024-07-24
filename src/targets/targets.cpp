#include "targets.h"

struct TargetInfo get_target_infos(std::string targetTriplet){
    struct TargetInfo targetInfo;
    targetInfo.llvm_target_triple = "";
    targetInfo.features = "";
    if (targetTriplet == "aarch64-linux-android"){
        targetInfo = aarch64_linux_android_get_target_infos();
    } else if (targetTriplet == "aarch64-unknown-none-softfloat"){
        targetInfo = aarch64_unknown_none_softfloat_get_target_infos();
    } else if (targetTriplet == "i686-linux-android"){
        targetInfo = i686_linux_android_get_target_infos();
    } else if (targetTriplet == "i686-unknown-uefi"){
        targetInfo = i686_unknown_uefi_get_target_infos();
    } else if (targetTriplet == "loongarch64-unknown-linux-gnu"){
        targetInfo = loongarch64_unknown_linux_gnu_get_target_infos();
    } else if (targetTriplet == "loongarch64-unknown-none"){
        targetInfo = loongarch64_unknown_none_get_target_infos();
    } else if (targetTriplet == "mips-unknown-linux-gnu"){
        targetInfo = mips_unknown_linux_gnu_get_target_infos();
    } else if (targetTriplet == "wasm64-unknown-unknown"){
        targetInfo = wasm64_unknown_unknown_get_target_infos();
    } else if (targetTriplet == "x86_64-linux-android"){
        targetInfo = x86_64_linux_android_get_target_infos();
    } else if (targetTriplet == "x86_64-pc-windows-gnu"){
        targetInfo = x86_64_pc_windows_gnu_get_target_infos();
    } else if (targetTriplet == "x86_64-unknown-none"){
        targetInfo = x86_64_unknown_none_get_target_infos();
    } else if (targetTriplet == "x86_64-unknown-uefi"){
        targetInfo = x86_64_unknown_uefi_get_target_infos();
    }
    if (targetInfo.llvm_target_triple == ""){
        targetInfo.llvm_target_triple = targetTriplet;
    }
    return targetInfo;
}