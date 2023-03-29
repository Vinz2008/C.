
#include <iostream>
#include <filesystem>
#include "cli.h"

namespace fs = std::filesystem;

void updateGitRepo(std::string folder){
    std::string cmd = "git -C ";
    cmd += folder;
    cmd += " pull";
    auto returnGit = runCommand(cmd);
    std::cout << folder << " : " << returnGit->buffer << std::endl;
}

void cloneGit(std::string url, std::string username, std::string repo_name, std::string folder){
    if (folder.back() != '/'){
        folder += '/';
    }
    folder += repo_name;
    fs::path f{ folder };
    if (fs::exists(f)){
        updateGitRepo(folder);
        return;
    }
    std::string cmd = "git clone ";
    cmd += (url + username + '/' + repo_name);
    cmd += " ";
    cmd += folder;
    std::cout << "git cmd : " << cmd << std::endl;
    auto returnGit = runCommand(cmd);
    std::cout << "out : " << returnGit->buffer << std::endl;
}

void cloneGithub(std::string username, std::string repo_name, std::string folder){
    cloneGit("https://github.com/", username, repo_name, folder);
}