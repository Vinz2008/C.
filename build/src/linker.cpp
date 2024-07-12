#include "linker.h"
#include "cli.h"
#include <filesystem>
#include "../../src/config.h"
#include "log.h"

namespace fs = std::filesystem;

bool shouldRebuildSTD(std::string std_path, std::string target, bool is_gc);
void writeLastBuildToml(std::string std_path, std::string target, bool is_gc);

void linkFiles(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot, bool is_gc, bool is_strip_mode, std::string linker_path){
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
    if (linker_path != ""){
        cmd.append(" -fuse-ld=" + linker_path + " ");
    }
    if (doesExeExist("mold")){
        cmd.append(" -fuse-ld=mold ");
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
    cmd.append(" -lm ");
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
    Log() << "exe link cmd : " << cmd << "\n";
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
    std::cout << "lib link cmd : " << cmd << std::endl; // TODO : remove this / make it debug output
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
    std::cout << "dynlib link cmd : " << cmd << std::endl; // TODO : remove this / make it debug output
    runCommand(cmd);
}