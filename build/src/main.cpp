#include "../../src/config.h"
#include "cli.h"
#include "files.h"
#include "dependencies.h"
#include "project_creator.h"
#include <toml++/toml.h>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

void buildSubfolders(toml::v3::table& config, std::string_view type, std::string target, std::string sysroot);
void buildFolder(std::string src_folder, toml::v3::table& config,std::string_view type, std::string target, std::string sysroot);

enum mode {
    BUILD_MODE = -1,
    CLEAN_MODE = -2,
    INFO_MODE = -3,
    DOWNLOAD_MODE = -4,
    ADD_DEPENDENCY_MODE = -5,
    OPEN_PAGE_MODE = -6,
    NEW_PROJECT_MODE = -7,
    CROSS_COMPILE_MODE = -8,
};

std::string filename_config = "build.toml";
std::vector<std::string> PathList;

void downloadDependencies(toml::v3::table config){
    auto github_dependencies = config["dependencies"]["github"];
    if (toml::array* arr = github_dependencies.as_array()){
        arr->for_each([](auto&& dep){
            if constexpr (toml::is_string<decltype(dep)>){
            std::string dependency = *dep;
            std::string username = dependency.substr(0, dependency.find('/'));
            std::string repo_name = dependency.substr(dependency.find('/')+1, dependency.size());
            cloneGithub(username, repo_name, DEFAULT_PACKAGE_PATH);
            }
        });
    }
}

void addDependency(std::string dependency_name, toml::v3::table& config){
    auto github_dependencies = config["dependencies"]["github"];
    if (toml::array* arr = github_dependencies.as_array()){
        arr->for_each([dependency_name](auto&& dep){
            if constexpr (toml::is_string<decltype(dep)>){
                std::cout << "add dependency : " << dependency_name << std::endl;
                std::cout << "dep : " << dep << std::endl;
            }
        });
        arr->insert(arr->end(), dependency_name);
        //std::cout << config << std::endl; 
        std::ofstream configFstream(filename_config);
        configFstream << config;
        configFstream.close();
    }
}

void buildSubfolders(toml::v3::table& config, std::string_view type, std::string target, std::string sysroot){
    auto subfolders = config["subfolders"]["folders"];
    if (toml::array* arr = subfolders.as_array()){
        arr->for_each([&config, type, target, sysroot](auto&& sub){
            if constexpr (toml::is_string<decltype(sub)>){
                std::cout << "sub : " << sub << std::endl;
                buildFolder((std::string)sub, config, type, target, sysroot);
            }
        });
    }
}

void buildFolder(std::string src_folder, toml::v3::table& config, std::string_view type, std::string target, std::string sysroot){
    std::string_view arguments_view = config["build"]["arguments"].value_or("");
    std::string arguments = (std::string) arguments_view;
    if (type == "dynlib"){
        arguments += " -fPIC ";
    }
    std::vector<std::string> localPathList = getFilenamesWithExtension(".cpoint", src_folder);
    PathList.insert(PathList.end(), localPathList.begin(), localPathList.end());
    for (auto const& path : localPathList){
        std::cout << path << ' ';
        compileFile(target, "-no-gc" + arguments, path, sysroot);
    }
    std::cout << std::endl;
}

void runCustomScripts(toml::v3::table& config){
    auto scripts = config["custom"]["scripts"];
    if (toml::array* arr = scripts.as_array()){
        arr->for_each([](auto&& script){
            if constexpr (toml::is_string<decltype(script)>){
                std::cout << "script : " << script << std::endl;
                std::unique_ptr<ProgramReturn> returnScript = runCommand((std::string) script);
                std::cout << returnScript->buffer << std::endl;
            }
        });
    }
}

void addCustomLinkableFiles(toml::v3::table& config){
    auto linkableFiles = config["custom"]["linkablefiles"];
    if (toml::array* arr = linkableFiles.as_array()){
        arr->for_each([](auto&& file){
            if constexpr (toml::is_string<decltype(file)>){
                PathList.push_back((std::string) file);
            }
        });
    }
}

void runPrebuildCommands(toml::v3::table& config){
    auto commands = config["custom"]["prebuild_commands"];
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
        std::string object_path = path_fs.replace_extension(".o");
        std::cout << object_path << ' ';
        remove(object_path.c_str());
    }
    std::cout << std::endl;
}

void cleanObjectFiles(toml::v3::table& config, std::string src_folder){
    cleanObjectFilesFolder(src_folder);
    auto subfolders = config["subfolders"]["folders"];
    if (toml::array* arr = subfolders.as_array()){
        arr->for_each([&config](auto&& sub){
            if constexpr (toml::is_string<decltype(sub)>){
                cleanObjectFilesFolder((std::string) sub);
            }
        });
    }
    runCustomCleanCommands(config);
}

int main(int argc, char** argv){
    enum mode modeBuild = BUILD_MODE;
    std::string src_folder = "src/";
    std::string dependency_to_add = "";
    std::string project_name_to_create = "";
    for (int i = 0; i < argc; i++){
    std::string arg = argv[i];
    if (arg == "-f"){
    i++;
    filename_config = argv[i];
    } else if (arg == "build"){
        modeBuild = BUILD_MODE;
    } else if (arg == "clean"){
        modeBuild = CLEAN_MODE;
    } else if (arg == "info"){
        modeBuild = INFO_MODE;
    } else if (arg == "download" || arg == "update"){
        modeBuild = DOWNLOAD_MODE;
    } else if (arg == "add"){
        modeBuild = ADD_DEPENDENCY_MODE;
        i++;
        dependency_to_add = argv[i];
    } else if (arg == "new"){
        modeBuild = NEW_PROJECT_MODE;
        i++;
        project_name_to_create = argv[i];
    } else if (arg == "open-page"){
        modeBuild = OPEN_PAGE_MODE;
        i++;
    } else if (arg == "cross-compile"){
        modeBuild = CROSS_COMPILE_MODE;
        i++;
    }
    }
    // instruction that don't need the toml file
    if (modeBuild == NEW_PROJECT_MODE){
        createProject("exe", project_name_to_create);
        return 0;
    }
    fs::path f{ filename_config };
    if (!fs::exists(f)){
        std::cout << "Could not find " << filename_config << " file" << std::endl;
        exit(1);
    }
    auto config = toml::parse_file(filename_config);
    std::string_view src_folder_temp = config["project"]["src_folder"].value_or("");
    std::string_view type = config["project"]["type"].value_or("");
    std::string_view target_view_cross = config["cross-compile"]["target"].value_or("");  
    std::string target_cross = (std::string)target_view_cross; 
    std::string linker_args = (std::string)config["build"]["linker_arguments"].value_or("");   
    std::string cross_compile_linker_args = (std::string)config["cross-compile"]["linker_arguments"].value_or("");
    std::string sysroot_cross = (std::string)config["cross-compile"]["sysroot"].value_or("");   
    if (src_folder_temp != ""){
        src_folder = src_folder_temp;
    }
    PathList.clear();
    if (modeBuild == CLEAN_MODE){
        cleanObjectFiles(config, src_folder);
    } else if (modeBuild == INFO_MODE){
        std::string_view project_name = config["project"]["name"].value_or("");
        std::cout << "project name : " << project_name << "\n";
        std::string_view homepage = config["project"]["homepage"].value_or("");
        std::cout << "homepage : " << homepage << "\n";
        std::string_view license = config["project"]["license"].value_or("");
        std::cout << "license : " << license << "\n";
        std::cout << "type : " << type << "\n";
        std::cout << "src folder : " << src_folder_temp << "\n";
    } else if (modeBuild == DOWNLOAD_MODE){
        downloadDependencies(config);
    } else if (modeBuild == ADD_DEPENDENCY_MODE){
        addDependency(dependency_to_add, config);
    } else if (modeBuild == OPEN_PAGE_MODE){
        std::string_view homepage = config["project"]["homepage"].value_or("");
        openWebPage((std::string) homepage);
    } else if (modeBuild == BUILD_MODE || modeBuild == CROSS_COMPILE_MODE) {
        std::string_view outfilename_view = config["build"]["outfile"].value_or("");
        std::string outfilename = (std::string) outfilename_view;
        std::string target = "";
        std::string sysroot = "";
        if (modeBuild == CROSS_COMPILE_MODE){
        linker_args += cross_compile_linker_args;
        target = target_cross;
        sysroot = sysroot_cross;
        }
        if (modeBuild != CROSS_COMPILE_MODE){
            downloadDependencies(config);
            runPrebuildCommands(config);
        }
        buildSubfolders(config, type, target, sysroot);
        buildFolder(src_folder, config, type, target, sysroot);
        runCustomScripts(config);
        if (modeBuild != CROSS_COMPILE_MODE){
            addCustomLinkableFiles(config);
        }
        if (type == "exe"){
        linkFiles(PathList, outfilename, target, linker_args, sysroot);
        } else if (type == "library"){
        linkLibrary(PathList, outfilename, target, linker_args, sysroot);
        } else if (type == "dynlib"){
            linkDynamicLibrary(PathList, outfilename, target, linker_args, sysroot);
        }
    }
}
