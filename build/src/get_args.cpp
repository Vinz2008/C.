#include "get_args.h"
#include <iostream>
#include <algorithm>
#include "cli.h"
#include "../../src/gettext.h"

static std::string get_pkg_config_linker_args(std::string library_name){
    std::string cmd = "pkg-config --libs " + library_name;
    auto cmd_out = runCommand(cmd);
    return cmd_out->buffer;
}

static std::string get_llvm_config_linker_args(){
    std::string cmd = "llvm-config --ldflags --system-libs --libs core";
    auto cmd_out = runCommand(cmd);
    std::string ret = cmd_out->buffer;
    std::replace(ret.begin(), ret.end(), '\n', ' ');
    std::cout << "ret : " << ret << std::endl; // TODO : remove this / make it debug output
    return ret;
}

/*static std::string get_pkg_config_cflags_args(std::string library_name){
    std::string cmd = "pkg-config --cflags " + library_name;
    auto cmd_out = runCommand(cmd);
    return cmd_out->buffer;
}*/

void handle_library_name(std::string library, std::string& linker_args){
    if (library == "pthread"){
        linker_args += "-lpthread ";
    } else if (library == "gtk"){
        linker_args += get_pkg_config_linker_args("gtk4");
    } else if (library == "raylib" || library == "lua"){
        linker_args += get_pkg_config_linker_args((std::string)library);
    } else if (library == "llvm"){
        linker_args += get_llvm_config_linker_args();
    } else if (library == "clang"){
        linker_args += "-lclang";
    } else {
        std::cout << _("Warning : unknown library : ") << library << std::endl;
        linker_args += get_pkg_config_linker_args((std::string)library);
    }
}