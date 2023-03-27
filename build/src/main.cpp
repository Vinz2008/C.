#include "../../src/config.h"
#include "cli.h"
#include "files.h"
#include <toml++/toml.h>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

enum mode {
    BUILD_MODE = -1,
    CLEAN_MODE = -2,
};

int main(int argc, char** argv){
    enum mode modeBuild = BUILD_MODE;
    std::string filename_config = "build.toml";
    std::string src_folder = "src";
    for (int i = 0; i < argc; i++){
    std::string arg = argv[i];
    if (arg == "-f"){
    i++;
    filename_config = argv[i];
    } else if (arg == "build"){
        modeBuild = BUILD_MODE;
    } else if (arg == "clean"){
        modeBuild = CLEAN_MODE;
    }
    }
    fs::path f{ filename_config };
    if (!fs::exists(f)){
        std::cout << "Could not find " << filename_config << " file" << std::endl;
        exit(1);
    }
    auto config = toml::parse_file(filename_config);
    std::string_view project_name = config["project"]["name"].value_or("");
    std::cout << "project name : " << project_name << "\n";
    std::string_view src_folder_temp = config["project"]["src_folder"].value_or("");
    if (src_folder_temp != ""){
        src_folder = src_folder_temp;
    }
    if (modeBuild == CLEAN_MODE){
        std::vector<std::string> PathList = getFilenamesWithExtension(".cpoint", "src/");
        for (auto const& path : PathList){
            fs::path path_fs{ path };
            std::string object_path = path_fs.replace_extension(".o");
            std::cout << object_path << ' ';
            remove(object_path.c_str());
        }
    } else if (modeBuild == BUILD_MODE) {
    std::vector<std::string> PathList = getFilenamesWithExtension(".cpoint", "src/");
    for (auto const& path : PathList){
        std::cout << path << ' ';
        compileFile("", "", path);
    }
    std::cout << std::endl;
    }

}
