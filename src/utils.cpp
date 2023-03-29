#include <fstream>
#include <iostream>
#include <sys/stat.h>

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