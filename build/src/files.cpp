#include "files.h"
#include <iostream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

std::vector<std::string> getFilenamesWithExtension(std::string extension, std::string folder){
    fs::path startPath{ folder };
    std::vector<std::string> fileNameList;
    std::vector fileList (fs::recursive_directory_iterator(startPath), {});
    for (const fs::directory_entry& de : fileList) {

        // Get path as string
        if (!de.is_directory()){
        std::string p{ de.path().string() };

        // Check if it ends with the given extension
        if (p.find(".git") == std::string::npos){
        //std::cout << "p : " << p << std::endl;
        //std::cout << "p.size() - extension.size() : " << p.size() - extension.size() << std::endl;;
        if (p.size() - extension.size() != std::string::npos && p.size() > extension.size() && p.substr(p.size() - extension.size(), extension.size()) == extension)
            fileNameList.push_back(p);
        }
        }
    }
    return fileNameList;
}
