#include "build.h"
#include <iostream>
#include <thread>
#include "files.h"
#include "cli.h"
#define  TOML_HEADER_ONLY 1
#include <toml++/toml.h>
#include <filesystem>

namespace fs = std::filesystem;


bool shouldCompileFile(std::string file){
    fs::path path{file};
    path.replace_extension("o");
    if (!fs::exists(path)){
        return true;
    }
    std::string object_file = path.string();
    auto file_timestamp = fs::last_write_time((fs::path)(file)).time_since_epoch();
    auto object_file_timestamp = fs::last_write_time((fs::path)(object_file)).time_since_epoch();
    return (file_timestamp.count() > object_file_timestamp.count());
}


std::vector<std::string> buildFileEachThreadPathList;

void buildFileEachThread(int index, std::string target, std::string sysroot, std::string arguments){
    std::string path = buildFileEachThreadPathList.at(index);
    std::cout << path << ' ';
    compileFile(target, /*"-no-gc" +*/ arguments, path, sysroot);
}

void buildFolderMultiThreaded(std::string src_folder, toml::v3::table& config, std::string_view type, std::string target, std::string sysroot, bool is_gc, int thread_number, std::vector<std::string> localPathList, std::string arguments){
    std::cout << "Multi threaded with " << thread_number << " threads" << std::endl;
    std::vector<std::thread> threads;
    buildFileEachThreadPathList = localPathList;
    for (int i = 0; i < localPathList.size(); i+=thread_number){
    for (int j = i; j < i+thread_number; j++){
        if (j < localPathList.size()){
        threads.push_back(std::thread(buildFileEachThread, j, target, sysroot, arguments));
        }
    }
    }
    for(auto& thread : threads){
        thread.join();
    }
}

extern std::vector<std::string> PathList;

void buildFolder(std::string src_folder, toml::v3::table& config, std::string_view type, std::string target, std::string sysroot, bool is_gc, int thread_number){
    std::cout << "buildFolder threads " << thread_number  << std::endl;
    std::string_view arguments_view = config["build"]["arguments"].value_or("");
    std::string arguments = (std::string) arguments_view;
    if (type == "dynlib"){
        arguments += " -fPIC ";
    }
    std::vector<std::string> localPathList = getFilenamesWithExtension(".cpoint", src_folder);
    PathList.insert(PathList.end(), localPathList.begin(), localPathList.end());
    if (!is_gc){
        arguments += " -no-gc ";
    }
    if (thread_number > 1){
        buildFolderMultiThreaded(src_folder, config, type, target, sysroot, is_gc, thread_number, localPathList, arguments);
    } else {
    for (auto const& path : localPathList){
        if (shouldCompileFile(path)){
            std::cout << path << ' ';
            compileFile(target, /*"-no-gc" +*/ arguments, path, sysroot);
        }
    }
    }
    std::cout << std::endl;
}