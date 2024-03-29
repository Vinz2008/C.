#include "build.h"
#include <iostream>
#include <thread>
#include "files.h"
#include "cli.h"
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

void buildSubfolders(toml::v3::table& config, std::string_view type, std::string target, std::string sysroot, bool is_gc, int thread_number){
    auto subfolders = config["subfolders"]["folders"];
    if (toml::array* arr = subfolders.as_array()){
        int subfolder_number = arr->size();
        arr->for_each([&config, type, target, sysroot, is_gc, subfolder_number,thread_number](auto&& sub){
            if constexpr (toml::is_string<decltype(sub)>){
                std::cout << "sub : " << sub << std::endl;
                buildFolder((std::string)sub, config, type, target, sysroot, is_gc, (int)lround(thread_number/subfolder_number));
            }
        });
    }
}

bool isSubprojectGC(std::string path){
    //std::cout << "project gc path : " << path + "/build.toml" << std::endl;
    auto subproject_config = toml::parse_file(path + "/build.toml");
    bool is_gc = subproject_config["build"]["gc"].value_or(true);
    //std::cout << "project gc : " << is_gc << std::endl;
    return is_gc;
}

void buildSubproject(std::string path){
    runCommand("cd " + path + " && cpoint-build");
}

void buildSubprojects(toml::v3::table& config){
    std::vector<std::string> no_gc_subprojects;
    std::vector<std::string> gc_subprojects;
    auto subfolders = config["subfolders"]["projects"];
    if (toml::array* arr = subfolders.as_array()){
        arr->for_each([&config, &gc_subprojects, &no_gc_subprojects](auto&& sub){
            if constexpr (toml::is_string<decltype(sub)>){
                //std::cout << "sub : " << sub << std::endl;
                //buildSubproject((std::string)sub);
                
                if (isSubprojectGC((std::string)sub)){
                    gc_subprojects.push_back((std::string)sub);
                } else {
                    no_gc_subprojects.push_back((std::string)sub);
                }
                //subprojects.push_back((std::string)sub);
            }
        });
    }
    for (int i = 0; i < no_gc_subprojects.size(); i++){
        //std::cout << "building no gc subprojects" << std::endl;
        std::cout << "sub : " << no_gc_subprojects.at(i) << std::endl;
        buildSubproject(no_gc_subprojects.at(i));
    }
    for (int i = 0; i < gc_subprojects.size(); i++){
        //std::cout << "building gc subprojects" << std::endl;
        std::cout << "sub : " << gc_subprojects.at(i) << std::endl;
        buildSubproject(gc_subprojects.at(i));
    }
}