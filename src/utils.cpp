#include <fstream>
#include <iostream>
#include <dirent.h>
#include <functional>
#include <sys/stat.h>
#include <filesystem>
#include <string.h>

namespace fs = std::filesystem;


bool FileExists(std::string filename){
    std::ifstream file(filename);
    if(file.is_open()){
    return 1;
    file.close();
    } else {
    return 0;
    }
}
bool FolderExists(std::string foldername){
    struct stat sb;
    if (stat(foldername.c_str(), &sb) == 0){
        std::cout << "folder exists" << std::endl;
        return true;
    } else {
        return false;
    }
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

void listFiles(const std::string &path, std::function<void(const std::string &)> cb) {
    for (const fs::directory_entry& dir_entry : fs::recursive_directory_iterator(path)){
        if (dir_entry.is_regular_file()){
            cb(dir_entry.path().string());
        }
    }
}

std::string getFileExtension(std::string path){
    for (int i = path.size()-1; i > 0; i--){
        if (path.at(i) == '.'){
            return path.substr(i+1, path.size()-(i+1));
        }
    }
    return "";
}