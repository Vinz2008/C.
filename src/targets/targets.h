#include <string>

struct TargetInfo {
    std::string llvm_target_triple;
    std::string features;
    // TODO : add data layout and the pointer width so it will be needed to ask llvm for them
};

struct TargetInfo get_target_infos(std::string targetTriplet);

struct TargetInfo aarch64_linux_android_get_target_infos();
struct TargetInfo aarch64_unknown_none_softfloat_get_target_infos();
struct TargetInfo i686_linux_android_get_target_infos();
struct TargetInfo i686_unknown_uefi_get_target_infos();
struct TargetInfo loongarch64_unknown_linux_gnu_get_target_infos();
struct TargetInfo loongarch64_unknown_none_get_target_infos();
struct TargetInfo mips_unknown_linux_gnu_get_target_infos();
struct TargetInfo wasm64_unknown_unknown_get_target_infos();
struct TargetInfo x86_64_linux_android_get_target_infos();
struct TargetInfo x86_64_pc_windows_gnu_get_target_infos();
struct TargetInfo x86_64_unknown_none_get_target_infos();
struct TargetInfo x86_64_unknown_uefi_get_target_infos();