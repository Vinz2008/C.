#include "targets.h"
#include <cstdlib>

TargetInfo aarch64_apple_darwin_get_target_infos(){
    std::string llvm_target_triple = "";
    // TODO : create a generic function in an apple.cpp file in a base folder for this code (it will be needed for all apple targets)
    // see https://github.com/rust-lang/rust/blob/master/compiler/rustc_target/src/spec/base/apple/mod.rs#L273
    std::string major = "11";
    std::string minor = "0";
    char* macos_deployment_target = std::getenv("MACOSX_DEPLOYMENT_TARGET");
    if (macos_deployment_target){
        std::string macos_deployment_target_str = macos_deployment_target;
        int delimiter_pos = macos_deployment_target_str.find(".");
        major = macos_deployment_target_str.substr(0, delimiter_pos);
        minor = macos_deployment_target_str.substr(delimiter_pos+1, macos_deployment_target_str.size()-delimiter_pos);
    }
    return TargetInfo {
        .llvm_target_triple = "arm64-apple-macosx" + major + "." + minor + ".0",
        .pointer_size = 64,
        .cpu = "apple-m1",
        .features = "",
    };
}