#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include "config.h"
#include "utils.h"
#include "cli.h"
#include "errors.h"


using namespace std;

extern bool debug_info_mode;

extern std::unique_ptr<Compiler_context> Comp_context;

#ifdef _WIN32
#include "windows.h"
#endif


int build_std(string path, string target_triplet, bool verbose_std_build, bool use_native_target, bool is_gc){
    string cmd_clean = "make -C " + path + " clean";
    if (Comp_context->debug_mode){
    cout << "cmd clean : " << cmd_clean << endl;
    }
    auto out_clean = runCommand(cmd_clean);
    if (verbose_std_build){
    cout << out_clean->buffer << endl;
    }
    int retcode = -1;
    string cmd = "";
    if (!use_native_target){
        cmd += "TARGET=" + target_triplet;   
    }
    if (!is_gc){
        cmd += "NO_GC=TRUE ";
    }
    cmd += " make -C " + path;
    if (debug_info_mode){
        cmd += " debug";
    }
    if (Comp_context->debug_mode){
    cout << "cmd : " << cmd << endl;
    }
    auto out = runCommand(cmd);
    retcode = out->exit_status;
    if (verbose_std_build){
    cout << out->buffer << endl;
    cout << "retcode : " << retcode << endl;
    }
    return retcode;
}
int build_gc(string path, string target_triplet){
    std::string prefix_path = path + "/../bdwgc_prefix";
    std::string makefile_path = path + "/Makefile";
    if (!FileExists(makefile_path)){
    std::string cmd_configure = "cd " + path + " && ./autogen.sh && ./configure --enable-static --host=" + target_triplet + " --prefix=";
    std::string complete_string_path = realpath(prefix_path.c_str(), NULL);
    cmd_configure += complete_string_path;
    cout << "cmd_configure : " << cmd_configure << endl;
    auto out_configure = runCommand(cmd_configure);
    cout << out_configure->buffer << endl;
    } else {
        std::string cmd_clean = "make -C " + path + " clean";
        auto out_clean = runCommand(cmd_clean);
        cout << out_clean->buffer << endl;
    }
    std::string cmd_make = "make -C " + path;
    cout << "cmd_make : " << cmd_make << endl;
    auto out = runCommand(cmd_make);
    cout << out->buffer << endl;
    std::string cmd_install = cmd_make + " install";
    cout << "cmd_install : " << cmd_install << endl;
    auto out_install = runCommand(cmd_install);
    cout << out_install->buffer << endl;
    std::cout << "end build gc" << std::endl;
    return 0;
}

void link_files(vector<string> list_files, string filename_out, string target_triplet, std::string linker_additional_flags){
    int retcode = -1;
    string cmd = "clang -o " + filename_out + " -target " + target_triplet;
    if (target_triplet.find("wasm") != string::npos){
        cmd += "-Wl,--export-all --no-standard-libraries -Wl,--no-entry ";
    } else {
        cmd += " -lm ";
    }
    if (doesExeExist("mold") && !Comp_context->lto_mode){
        cmd += " -fuse-ld=mold ";
    }
    cmd += linker_additional_flags;
    for (int i = 0; i < list_files.size(); i++){
        cmd += " " + list_files.at(i);
    }
    if (Comp_context->debug_mode){
        cout << "cmd : " << cmd << endl;
    }
    auto out = runCommand(cmd);
    retcode = out->exit_status;
    if (Comp_context->debug_mode){
        cout << "retcode : " << retcode << endl;
    }
    if (retcode == -1 || retcode == 1){
        cout << "Could not create executable " << filename_out << endl;
        exit(1);
    }
}
