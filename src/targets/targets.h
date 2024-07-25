#include <string>

struct TargetInfo {
    std::string llvm_target_triple;
    // the size is in bits
    int pointer_size;
    // the default cpu for the target
    std::string cpu;
    std::string features;
    // TODO : add data layout and the pointer width so it will be needed to ask llvm for them
};

TargetInfo get_target_infos(std::string targetTriplet);

TargetInfo aarch64_apple_darwin_get_target_infos();
TargetInfo aarch64_linux_android_get_target_infos();
TargetInfo aarch64_unknown_fuchsia_get_target_infos();
TargetInfo aarch64_unknown_none_softfloat_get_target_infos();
TargetInfo aarch64_unknown_uefi_get_target_infos();
TargetInfo i586_unknown_linux_gnu_get_target_infos();
TargetInfo i686_linux_android_get_target_infos();
TargetInfo i686_unknown_linux_gnu_get_target_infos();
TargetInfo i686_unknown_openbsd_get_target_infos();
TargetInfo i686_unknown_redox_get_target_infos();
TargetInfo i686_unknown_uefi_get_target_infos();
TargetInfo loongarch64_unknown_linux_gnu_get_target_infos();
TargetInfo loongarch64_unknown_none_get_target_infos();
TargetInfo mips_unknown_linux_gnu_get_target_infos();
TargetInfo s390x_unknown_linux_gnu_get_target_infos();
TargetInfo wasm64_unknown_unknown_get_target_infos();
TargetInfo x86_64_linux_android_get_target_infos();
TargetInfo x86_64_pc_windows_gnu_get_target_infos();
TargetInfo x86_64_unknown_none_get_target_infos();
TargetInfo x86_64_unknown_uefi_get_target_infos();