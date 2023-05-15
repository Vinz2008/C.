#include "cli.h"
#include <iostream>
#include <memory>
#include <array>
#include <vector>
#include <filesystem>
#include "../../src/config.h"

namespace fs = std::filesystem;

std::unique_ptr<ProgramReturn> runCommand(const std::string cmd){
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

}

void openWebPage(std::string url){
#ifdef _WIN32
    std::string cmd = "start ";
#else
    std::string cmd = "xdg-open ";
#endif
    cmd.append(url);
    runCommand(cmd);
}

void rebuildSTD(std::string target, std::string path){
    runCommand("make -C " + path + " clean");
    std::string cmd_start = "";
    if (target != ""){
        cmd_start += "TARGET=" + target + " "; 
    }
    std::cout << runCommand(cmd_start + "make -C " + path)->buffer << std::endl;
}

void compileFile(std::string target, std::string arguments, std::string path, std::string sysroot){
    std::string cmd = "cpoint -c " + arguments + " " + path + " ";
    fs::path path_fs{ path };
    std::string out_path = path_fs.replace_extension(".o");
    cmd += "-o " + out_path;
    if (target != ""){
        cmd += " -target-triplet " + target + " ";
    }
    std::cout << "cmd : " << cmd << std::endl;
    runCommand(cmd);
}

void linkFiles(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot){
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
        std::string out_path = path_fs.replace_extension(".o");
        cmd += out_path + " ";
    }
    cmd += " " DEFAULT_STD_PATH "/libstd.a";
    if (target != ""){
        std::string path = DEFAULT_STD_PATH;
        rebuildSTD(target, path);
    }
    std::cout << "exe link cmd : " << cmd << std::endl;
    runCommand(cmd);
}

void linkLibrary(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot){
    if  (outfilename == ""){
        outfilename = "lib.a";
    }
    std::string cmd = "ar rcs" + outfilename + " ";
    for (int i = 0; i < PathList.size(); i++){
        fs::path path_fs{ PathList.at(i) };
        std::string out_path = path_fs.replace_extension(".o");
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
        std::string out_path = path_fs.replace_extension(".o");
        cmd += out_path + " ";
    }
    std::cout << "dynlib link cmd : " << cmd << std::endl;
    runCommand(cmd);
}