#include "std.h"
#include "cli.h"
#include "log.h"
#include <filesystem>

#define  TOML_HEADER_ONLY 1
#include <toml++/toml.h>

namespace fs = std::filesystem;


bool shouldRebuildSTD(std::string std_path, std::string target, bool is_gc){
    std::string last_build_toml_path = std_path + "/last_build.toml";
    if (!fs::exists(fs::path(std_path + "/libstd.a"))){
        return true;
    }
    if (fs::exists(fs::path(last_build_toml_path))){ 
        auto last_build_toml = toml::parse_file(last_build_toml_path);
        if (!last_build_toml["target"].is_value()){
            return true;
        }
        std::string last_target = last_build_toml["target"].value_or("");
        bool last_is_gc = last_build_toml["is_gc"].value_or(true);
        return (last_target != target || last_is_gc != is_gc);
    }
    return true;
}

void rebuildSTD(std::string target, std::string path, bool is_gc){
    runCommand("make -C " + path + " clean");
    std::string cmd_start = "";
    if (target != ""){
        cmd_start += "TARGET=" + target + " "; 
    }
    if (!is_gc){
        cmd_start += "NO_GC=TRUE ";
    }
    Log() << runCommand(cmd_start + "make -C " + path).buffer << "\n";
}