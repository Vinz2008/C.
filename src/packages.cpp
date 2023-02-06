#include <iostream>
#include <fstream>
#include <vector>
#include "packages.h"
#include "config.h"
#include "log.h"

std::vector<std::string> PackagesAdded;

void add_package(std::string name){
    PackagesAdded.push_back(name);
}

int download_package_github(std::string username, std::string reponame){
    std::string cmd_clean = "rm -rf " DEFAULT_PACKAGE_PATH;
    cmd_clean.append("/");
    cmd_clean.append(reponame);
    FILE* pipe_clean = popen(cmd_clean.c_str(), "r");
    pclose(pipe_clean);
    std::string cmd = "git clone https://github.com/";
    cmd.append(username);
    cmd.append("/");
    cmd.append(reponame);
    cmd.append(" " DEFAULT_PACKAGE_PATH);
    cmd.append("/");
    cmd.append(reponame);
    FILE* pipe = popen(cmd.c_str(), "r");
    char* out = (char*)malloc(10000 * sizeof(char));
    fread(out, 1, 10000, pipe);
    std::string out_cpp = out;
    std::cout << out_cpp << std::endl;
    pclose(pipe);
    free(out);
    return 0;
}


int build_package(std::string path){
    Log::Build_Info() << "Building package at " << path << "\n";
    std::string cmd = "make -C ";
    cmd.append(path);
    FILE* pipe = popen(cmd.c_str(), "r");
    char* out = (char*)malloc(100000 * sizeof(char));
    fread(out, 1, 100000, pipe);
    std::string out_cpp = out;
    std::cout << out_cpp << std::endl;
    pclose(pipe);
    free(out);
    return 0;
}