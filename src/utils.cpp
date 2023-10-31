#include <fstream>
#include <iostream>
#include <dirent.h>
#include <functional>
#include <sys/stat.h>
#include <filesystem>

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

// TODO use the c++17 filesystem api to make it compatible with windows msys
void listFiles(const std::string &path, std::function<void(const std::string &)> cb) {
    for (const fs::directory_entry& dir_entry : fs::recursive_directory_iterator(path)){
        if (dir_entry.is_regular_file()){
            cb(dir_entry.path().string());
        }
    }
    /*if (auto dir = opendir(path.c_str())) {
        while (auto f = readdir(dir)) {
            if (f->d_name[0] == '.') continue;
            if (f->d_type == DT_DIR)
                listFiles(path + "/" + f->d_name + "/", cb);

            if (f->d_type == DT_REG)
                cb(path + "/" + f->d_name);
        }
        closedir(dir);
    }*/
}

std::string getFileExtension(std::string path){
    for (int i = path.size()-1; i > 0; i--){
        if (path.at(i) == '.'){
            return path.substr(i+1, path.size()-(i+1));
        }
    }
    return "";
}