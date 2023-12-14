#include <iostream>
#include <fstream>
#include <vector>
#include "packages.h"
#include "config.h"
#include "log.h"
#include "cli.h"

std::vector<std::string> PackagesAdded;

void add_package(std::string name){
    PackagesAdded.push_back(name);
}

int download_package_github(std::string username, std::string reponame){
    std::string cmd_clean = "rm -rf " DEFAULT_PACKAGE_PATH;
    cmd_clean.append("/");
    cmd_clean.append(reponame);
    runCommand(cmd_clean);
    std::string cmd = "git clone https://github.com/";
    cmd.append(username);
    cmd.append("/");
    cmd.append(reponame);
    cmd.append(" " DEFAULT_PACKAGE_PATH);
    cmd.append("/");
    cmd.append(reponame);
    auto out = runCommand(cmd);
    std::cout << out->buffer << std::endl;
    return 0;
}

extern bool link_files_mode;

int build_package(std::string path){
    if (!link_files_mode){
        return 0;
    }
    Log::Build_Info() << "Building package at " << path << "\n";

    std::string cmd_build = "cd " + path + " && ";
    cmd_build += "cpoint-build build";

    auto out = runCommand(cmd_build);
    std::cout << out->buffer << std::endl;
    return 0;
}