#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include "config.h"
#include "utils.h"
#include "cli.h"


using namespace std;

extern bool debug_mode;
extern bool debug_info_mode;

#ifdef _WIN32
#include "windows.h"
#endif

int build_std(string path, string target_triplet, bool verbose_std_build){
    string cmd_clean = "make -C ";
    cmd_clean.append(path);
    cmd_clean.append(" clean");
    if (debug_mode){
    cout << "cmd clean : " << cmd_clean << endl;
    }
    auto out_clean = runCommand(cmd_clean);
    if (verbose_std_build){
    cout << out_clean->buffer << endl;
    }
    int retcode = -1;

    string cmd = "TARGET=";
    cmd.append(target_triplet);
    cmd.append(" make -C ");
    cmd.append(path);
    if (debug_info_mode){
        cmd += " debug";
    }
    if (debug_mode){
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
    std::string prefix_path = path;
    prefix_path.append("/../bdwgc_prefix");
    std::string makefile_path = path;
    makefile_path.append("/Makefile");
    if (!FileExists(makefile_path)){
    std::string cmd_configure = "cd ";
    cmd_configure.append(path);
    cmd_configure.append(" && ./autogen.sh && ./configure --enable-static --host=");
    cmd_configure.append(target_triplet);
    cmd_configure.append(" --prefix=");
    std::string complete_string_path = realpath(prefix_path.c_str(), NULL);
    cmd_configure.append(complete_string_path);
    cout << "cmd_configure : " << cmd_configure << endl;
    auto out_configure = runCommand(cmd_configure);
    cout << out_configure->buffer << endl;
    } else {
        std::string cmd_clean = "make -C ";
        cmd_clean.append(path);
        cmd_clean.append(" clean");
        auto out_clean = runCommand(cmd_clean);
        cout << out_clean->buffer << endl;
    }
    std::string cmd_make = "make -C ";
    cmd_make.append(path);
    cout << "cmd_make : " << cmd_make << endl;
    auto out = runCommand(cmd_make);
    cout << out->buffer << endl;
    std::string cmd_install = cmd_make;
    cmd_install.append(" install");
    cout << "cmd_install : " << cmd_install << endl;
    auto out_install = runCommand(cmd_install);
    cout << out_install->buffer << endl;
    std::cout << "end build gc" << std::endl;
    return 0;
}

void link_files(vector<string> list_files, string filename_out, string target_triplet, std::string linker_additional_flags){
    int retcode = -1;
    string cmd = "clang -o ";
    cmd.append(filename_out);
    cmd.append(" -target ");
    cmd.append(target_triplet);
    if (target_triplet.find("wasm") != string::npos){
        cmd.append("-Wl,--export-all --no-standard-libraries -Wl,--no-entry ");
    } else {
        cmd.append(" -lm ");
    }
    cmd.append(linker_additional_flags);
    for (int i = 0; i < list_files.size(); i++){
        cmd.append(" ");
        cmd.append(list_files.at(i));
    }
    if (debug_mode){
    cout << "cmd : " << cmd << endl;
    }
    auto out = runCommand(cmd);
    retcode = out->exit_status;
    if (debug_mode){
    cout << "retcode : " << retcode << endl;
    }
    if (retcode == -1 || retcode == 1){
        cout << "Could not create executable " << filename_out << endl;
        exit(1);
    }
}
