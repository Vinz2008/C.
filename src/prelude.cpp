#include "prelude.h"
#include <filesystem>
#include <iostream>
#include "imports.h"
#include "log.h"

namespace fs = std::filesystem;

static bool ends_with(std::string str, std::string suffix){
    return str.size() >= suffix.size() && str.compare(str.size()-suffix.size(), suffix.size(), suffix) == 0;
}

void include_prelude(std::string std_path){
    std::string core_path = std_path + "/core/";
    for (const auto & entry : fs::directory_iterator(core_path)){
        if (ends_with(entry.path().string(), ".cpoint")){
            std::string file_path = realpath(entry.path().string().c_str(), NULL);
            Log::Info() << "importing : " << file_path << "\n"; 
            import_file(file_path, true);
        }
    }
}