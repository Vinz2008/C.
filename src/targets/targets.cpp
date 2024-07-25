#include "targets.h"
#include <functional>

const std::unordered_map<std::string,  std::function<TargetInfo()>> TargetInfoFuncs = {
    {"aarch64-apple-darwin", aarch64_apple_darwin_get_target_infos},
    {"aarch64-linux-android", aarch64_linux_android_get_target_infos},
    {"aarch64-unknown-fuchsia", aarch64_unknown_fuchsia_get_target_infos},
    {"aarch64-unknown-none-softfloat", aarch64_unknown_none_softfloat_get_target_infos},
    {"aarch64-unknown-uefi", aarch64_unknown_uefi_get_target_infos},
    {"i586-unknown-linux-gnu", i586_unknown_linux_gnu_get_target_infos},
    {"i686-linux-android", i686_linux_android_get_target_infos},
    {"i686-unknown-linux-gnu", i686_unknown_linux_gnu_get_target_infos},
    {"i686-unknown-redox", i686_unknown_redox_get_target_infos},
    {"i686-unknown-openbsd", i686_unknown_openbsd_get_target_infos},
    {"i686-unknown-uefi", i686_unknown_uefi_get_target_infos},
    {"loongarch64-unknown-linux-gnu", loongarch64_unknown_linux_gnu_get_target_infos},
    {"loongarch64-unknown-none", loongarch64_unknown_none_get_target_infos},
    {"mips-unknown-linux-gnu", mips_unknown_linux_gnu_get_target_infos},
    {"s390x-unknown-linux-gnu", s390x_unknown_linux_gnu_get_target_infos},
    {"wasm64-unknown-unknown", wasm64_unknown_unknown_get_target_infos},
    {"x86_64-linux-android", x86_64_linux_android_get_target_infos},
    {"x86_64-pc-windows-gnu", x86_64_pc_windows_gnu_get_target_infos},
    {"x86_64-unknown-none", x86_64_unknown_none_get_target_infos},
    {"x86_64-unknown-uefi", x86_64_unknown_uefi_get_target_infos},
};

TargetInfo get_target_infos(std::string targetTriplet){
   TargetInfo targetInfo = TargetInfo {
        .llvm_target_triple = "",
        .pointer_size = 0,
        .cpu = "",
        .features = "",
    };
    if (TargetInfoFuncs.find(targetTriplet) != TargetInfoFuncs.end()){
        targetInfo = TargetInfoFuncs.at(targetTriplet)();
    }
    /*targetInfo.llvm_target_triple = "";
    targetInfo.features = "";
    targetInfo.cpu = "generic";
    targetInfo.pointer_size = 0;*/
    /*if (targetTriplet == "aarch64-linux-android"){
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
    }*/
    if (targetInfo.llvm_target_triple == ""){
        targetInfo.llvm_target_triple = targetTriplet;
    }
    if (targetInfo.cpu == ""){
        targetInfo.cpu = "generic";
    }
    return targetInfo;
}