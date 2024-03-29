#include "clean.h"
#include <iostream>
#include "cli.h"
#include "files.h"
#include <filesystem>

namespace fs = std::filesystem;

void runCustomCleanCommands(toml::v3::table& config){
    auto commands = config["custom"]["clean_commands"];
    if (toml::array* arr = commands.as_array()){
        arr->for_each([](auto&& cmd){
            if constexpr (toml::is_string<decltype(cmd)>){
                std::cout << "cmd : " << cmd << std::endl;
                std::unique_ptr<ProgramReturn> returnCmd = runCommand((std::string) cmd);
                std::cout << returnCmd->buffer << std::endl;
            }
        });
    }
}

void cleanObjectFilesFolder(std::string folder){
    std::vector<std::string> PathList = getFilenamesWithExtension(".cpoint", folder);
    for (auto const& path : PathList){
        fs::path path_fs{ path };
        
        std::string object_path = path_fs.replace_extension(".o").string();
        std::cout << object_path << ' ';
        remove(object_path.c_str());
    }
    std::cout << std::endl;
}

void cleanFilesFolderExtension(std::string folder, std::string extension){
    std::vector<std::string> PathList = getFilenamesWithExtension(extension, folder);
    for (auto const& path : PathList){
        std::string path_temp = path;
        remove(path_temp.c_str());
    }
}

void cleanFilesFolder(std::string folder){
    cleanFilesFolderExtension(folder, ".ll");
    cleanFilesFolderExtension(folder, ".test.o");
    cleanFilesFolderExtension(folder, ".test");
    cleanObjectFilesFolder(folder);
}


void cleanObjectFilesSubprojects(toml::v3::table& config){
    auto subfolders = config["subfolders"]["projects"];
    if (toml::array* arr = subfolders.as_array()){
        arr->for_each([&config](auto&& sub){
            if constexpr (toml::is_string<decltype(sub)>){
                std::cout << "sub : " << sub << std::endl;
                cleanFilesFolder((std::string)sub);
                runCommand("cd " + (std::string)sub + " && cpoint-build clean");
                //cleanObjectFilesFolder((std::string)sub);
            }
        });
    }
}

extern bool src_folder_exists;

void cleanFiles(toml::v3::table& config, std::string src_folder, std::string type){
    if (src_folder_exists){
    cleanFilesFolder(src_folder);
    //cleanObjectFilesFolder(src_folder);
    }
    auto subfolders = config["subfolders"]["folders"];
    if (toml::array* arr = subfolders.as_array()){
        arr->for_each([&config](auto&& sub){
            if constexpr (toml::is_string<decltype(sub)>){
                cleanFilesFolder((std::string)sub);
                //cleanObjectFilesFolder((std::string)sub);
            }
        });
    }
    cleanObjectFilesSubprojects(config);
    runCustomCleanCommands(config);
    std::cout << "type : " << type << std::endl;
    if (type == "exe"){
        std::string outfile = (std::string)config["build"]["outfile"].value_or("");
        if (outfile == ""){
            outfile = "a.out";
        }
        remove(outfile.c_str());
    }
}