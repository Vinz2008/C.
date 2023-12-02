#include "cli.h"
#include <iostream>
#include <memory>
#include <array>
#include <vector>
#include <filesystem>
#include <toml++/toml.h>
#include "../../src/config.h"

namespace fs = std::filesystem;

/*std::unique_ptr<ProgramReturn> runCommand(const std::string cmd){
    int exit_status = 0;
    auto pipe = popen(cmd.c_str(), "r");
    if (pipe == nullptr){
        throw std::runtime_error("Cannot open pipe");
    }
    std::array<char, 256> buffer;
    std::string result;

    while(not std::feof(pipe))
    {
        auto bytes = std::fread(buffer.data(), 1, buffer.size(), pipe);
        result.append(buffer.data(), bytes);
    }
    auto rc = pclose(pipe);
    if (WEXITSTATUS(rc)){
        exit_status = WEXITSTATUS(rc);
    }
    return std::make_unique<ProgramReturn>(exit_status, result);

}*/

bool shouldRebuildSTD(std::string std_path, std::string target, bool is_gc);
void writeLastBuildToml(std::string std_path, std::string target, bool is_gc);

void openWebPage(std::string url){
#ifdef _WIN32
    std::string cmd = "start ";
#else
    std::string cmd = "xdg-open ";
#endif
    cmd.append(url);
    runCommand(cmd);
}

void buildProject(std::string path){
    runCommand("cd " + path + " && cpoint-build");
}

void buildDependency(std::string path /*dependency*/){
    //std::string path = DEFAULT_PACKAGE_PATH "/" + dependency;
    buildProject(path);
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
    std::cout << runCommand(cmd_start + "make -C " + path)->buffer << std::endl;
}

void compileFile(std::string target, std::string arguments, std::string path, std::string sysroot){
    std::string cmd = "cpoint -c " + arguments + " " + path + " ";
    fs::path path_fs{ path };
    std::string out_path = path_fs.replace_extension(".o").string();
    cmd += "-o " + out_path;
    if (target != ""){
        cmd += " -target-triplet " + target + " ";
    }
    std::cout << "cmd : " << cmd << std::endl;
    runCommand(cmd);
}

void linkFiles(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot, bool is_gc, bool is_strip_mode){
    if  (outfilename == ""){
        outfilename = "a.out";
    }
    std::string cmd = "clang -o " + outfilename + " ";
    if (target != ""){
        cmd.append(" -target " + target + " ");
    }
    if (sysroot != ""){
        cmd.append(" --sysroot=" + sysroot + " ");
    }
    if (args != ""){
        cmd.append(" " + args + " ");
    }
    for (int i = 0; i < PathList.size(); i++){
        fs::path path_fs{ PathList.at(i) };
        std::string out_path;
        out_path = path_fs.string();
        if (path_fs.extension() == ".cpoint" || path_fs.extension() == ".c"){
            out_path = (std::string)path_fs.replace_extension(".o").string();
        } else {
            out_path = path_fs.string();
        }
        cmd += out_path + " ";
    }
    cmd += " " DEFAULT_STD_PATH "/libstd.a";
    if (is_gc){
        cmd += " " DEFAULT_STD_PATH "/../bdwgc_prefix/lib/libgc.a";
    }
    if (/*target != ""*/ shouldRebuildSTD(DEFAULT_STD_PATH, target, is_gc)){
        std::string path = DEFAULT_STD_PATH;
        //std::cout << "Rebuilding std ..." << std::endl;
        rebuildSTD(target, path, is_gc);
        //writeLastBuildToml(path, target, is_gc);
    }
    writeLastBuildToml(DEFAULT_STD_PATH, target, is_gc);
    std::cout << "exe link cmd : " << cmd << std::endl;
    runCommand(cmd);
    if (is_strip_mode){
        std::string cmd_strip = "llvm-strip " + outfilename;
        runCommand(cmd_strip);
    }
}

void linkLibrary(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot){
    if  (outfilename == ""){
        outfilename = "lib.a";
    }
    std::string cmd = "llvm-ar rcs " + outfilename + " ";
    for (int i = 0; i < PathList.size(); i++){
        fs::path path_fs{ PathList.at(i) };
        std::string out_path = (path_fs.extension() == ".a") ? path_fs.string() : path_fs.replace_extension(".o").string();
        cmd += out_path + " ";
    }
    std::cout << "lib link cmd : " << cmd << std::endl;
    runCommand(cmd);
}

// need to add an option in compiler to implement -fPIC
void linkDynamicLibrary(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot){
    if  (outfilename == ""){
        outfilename = "lib.so";
    }
    std::string cmd = "clang -shared -o " + outfilename + " ";
    for (int i = 0; i < PathList.size(); i++){
        fs::path path_fs{ PathList.at(i) };
        std::string out_path = path_fs.replace_extension(".o").string();
        cmd += out_path + " ";
    }
    std::cout << "dynlib link cmd : " << cmd << std::endl;
    runCommand(cmd);
}