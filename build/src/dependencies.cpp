
#include <iostream>
#include "cli.h"


void cloneGit(std::string url, std::string username, std::string repo_name, std::string folder){
    std::string cmd = "git clone ";
    cmd += (url + username + repo_name);
    cmd += " ";
    if (folder.back() != '/'){
        folder += '/';
    }
    folder += repo_name;
    cmd += folder;
    std::cout << "git cmd : " << cmd << std::endl;
    auto returnGit = runCommand(cmd);
    std::cout << "out : " << returnGit->buffer << std::endl;
}

void cloneGithub(std::string username, std::string repo_name, std::string folder){
    cloneGit("https://github.com/", username, repo_name, folder);
}