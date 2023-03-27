#include "files.h"
#include <iostream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

std::vector<std::string> getFilenamesWithExtension(std::string extension, std::string folder){
    fs::path startPath{ folder };
    std::vector<std::string> fileNameList;
    std::vector fileList (fs::directory_iterator(startPath), {});
    for (const fs::directory_entry& de : fileList) {

        // Get path as string
        std::string p{ de.path().string() };

        // Check if it ends with the given extension
        if (p.substr(p.size() - extension.size(), extension.size()) == extension)
            fileNameList.push_back(p);
    }
    return fileNameList;
}
