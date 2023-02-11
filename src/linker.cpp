#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include "config.h"

using namespace std;

#ifdef _WIN32
#include "windows.h"
#endif

bool FileExists(string filename){
    ifstream file(filename);
    if(file.is_open()){
    return 1;
    file.close();
    } else {
    return 0;
    }
}

int build_std(string path, string target_triplet, bool verbose_std_build){
    string cmd_clean = "make -C ";
    cmd_clean.append(path);
    cmd_clean.append(" clean");
    cout << "cmd clean : " << cmd_clean << endl;
    FILE* pipe_clean = popen(cmd_clean.c_str(), "r");
    char* out_clean = (char*)malloc(1000 * sizeof(char));
	fread(out_clean, 1, 1000, pipe_clean);
    string out_cpp_clean = out_clean;
    cout << out_cpp_clean << endl;
    pclose(pipe_clean);
    free(out_clean);
    int retcode = -1;

    string cmd = "TARGET=";
    cmd.append(target_triplet);
    cmd.append(" make -C ");
    cmd.append(path);
    cout << "cmd : " << cmd << endl;
    FILE* pipe = popen(cmd.c_str(), "r");
    char* out = (char*)malloc(10000000 * sizeof(char));
	fread(out, 1, 10000000, pipe);
    string out_cpp = out;
    if (verbose_std_build){
        cout << out_cpp << endl;
    }
    free(out);
    retcode = pclose(pipe);
    cout << "retcode : " << retcode << endl;
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
    FILE* pipe_configure = popen(cmd_configure.c_str(), "r");
    char* out_configure = (char*)malloc(10000000 * sizeof(char));
	fread(out_configure, 1, 10000000, pipe_configure);
    string out_configure_cpp = out_configure;
    cout << out_configure_cpp << endl;
    free(out_configure);
    pclose(pipe_configure);
    } else {
        std::string cmd_clean = "make -C ";
        cmd_clean.append(path);
        cmd_clean.append(" clean");
        FILE* pipe_clean = popen(cmd_clean.c_str(), "r");
        char* out_clean = (char*)malloc(10000000 * sizeof(char));
	    fread(out_clean, 1, 10000000, pipe_clean);
        string out_clean_cpp = out_clean;
        cout << out_clean_cpp << endl;
        pclose(pipe_clean);
    }
    std::string cmd_make = "make -C ";
    cmd_make.append(path);
    cout << "cmd_make : " << cmd_make << endl;
    FILE* pipe_make = popen(cmd_make.c_str(), "r");
    char* out = (char*)malloc(10000000 * sizeof(char));
	fread(out, 1, 10000000, pipe_make);
    string out_cpp = out;
    cout << out_cpp << endl;
    free(out);
    pclose(pipe_make);
    std::string cmd_install = cmd_make;
    cmd_install.append(" install");
    cout << "cmd_install : " << cmd_install << endl;
    FILE* pipe_install = popen(cmd_install.c_str(), "r");
    char* out_install = (char*)malloc(10000000 * sizeof(char));
	fread(out_install, 1, 10000000, pipe_install);
    string out_install_cpp = out_install;
    cout << out_install_cpp << endl;
    pclose(pipe_install);
    std::cout << "end build gc" << std::endl;
    return 0;
}

void link_files(vector<string> list_files, string filename_out, string target_triplet){
    int retcode = -1;
    string cmd = "clang -o ";
    cmd.append(filename_out);
    cmd.append(" -target ");
    cmd.append(target_triplet);
    if (target_triplet.find("wasm") != string::npos){
        //cmd.append(" --no-standard-libraries -Wl,--export-all -Wl,--no-entry ");
        cmd.append("-Wl,--export-all --no-standard-libraries -Wl,--no-entry");
    }
    for (int i = 0; i < list_files.size(); i++){
        cmd.append(" ");
        cmd.append(list_files.at(i));
    }
    cout << "cmd : " << cmd << endl;
    FILE* pipe = popen(cmd.c_str(), "r");
    retcode = pclose(pipe);
    cout << "retcode : " << retcode << endl;
    if (retcode == -1 || retcode == 1){
        cout << "Could not create executable " << filename_out << endl;
        exit(1);
    }
}
