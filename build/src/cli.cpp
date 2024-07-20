#include "cli.h"
#include "log.h"
#include <memory>
#include <array>
#include <vector>
#include <filesystem>
#include <toml++/toml.h>
//#include "../../src/config.h"

namespace fs = std::filesystem;

void openWebPage(std::string url){
#ifdef _WIN32
    std::string cmd = "start ";
#else
    std::string cmd = "xdg-open ";
#endif
    cmd.append(url);
    runCommand(cmd);
}


std::vector<std::string> PATHS;

bool doesExeExist(std::string filename){
    if (PATHS.empty()){
        char* PATH = getenv("PATH");
        PATH = strdup(PATH);
        char *path_string = strtok(PATH, ":");
        while (path_string != NULL) {
            PATHS.push_back(path_string);
            path_string = strtok(NULL, ":");
        }
        free(PATH);
    }
    for (int i = 0; i < PATHS.size(); i++){
        std::string exepath = PATHS.at(i) + "/" + filename;
        if (fs::exists(exepath)){
            return true;
        }
    }
    return false;
}

static void buildProject(std::string path){
    runCommand("cd " + path + " && cpoint-build");
}

void buildDependency(std::string path){
    buildProject(path);
}

std::string getCpointCompiler(){
    std::string compiler = "cpoint";
    if (fs::exists("../cpoint")){
        compiler = "../cpoint";
    }
    if (fs::exists("../../cpoint")){
        compiler = "../../cpoint";
    }
    return compiler;
}

void compileFile(std::string target, std::string arguments, std::string path, std::string sysroot, std::string out_path){
    std::string compiler = getCpointCompiler();
    std::string cmd = compiler + " -c " + arguments + " " + path + " ";
    fs::path path_fs{ path };
    if (out_path == ""){
        out_path = path_fs.replace_extension(".o").string();
    }
    cmd += "-o " + out_path;
    if (target != ""){
        cmd += " -target-triplet " + target + " ";
    }
    Log() << "cmd : " << cmd << "\n";
    runCommand(cmd);
}