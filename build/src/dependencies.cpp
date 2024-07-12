#include "dependencies.h"
#include <filesystem>
#include "cli.h"
#include "log.h"
#include "../../src/config.h"

namespace fs = std::filesystem;

void updateGitRepo(std::string folder){
    std::string cmd = "git -C ";
    cmd += folder;
    cmd += " pull";
    auto returnGit = runCommand(cmd);
    Log() << folder << " : " << returnGit->buffer << "\n";
}

void cloneGit(std::string url, std::string username, std::string repo_name, std::string folder){
    if (folder.back() != '/'){
        folder += '/';
    }
    folder += repo_name;
    fs::path f{ folder };
    if (fs::exists(f)){
        updateGitRepo(folder);
        return;
    }
    std::string cmd = "git clone ";
    cmd += (url + username + '/' + repo_name);
    cmd += " ";
    cmd += folder;
    Log() << "git cmd : " << cmd << "\n";
    auto returnGit = runCommand(cmd);
    Log() << "out : " << returnGit->buffer << "\n";
}

void cloneGithub(std::string username, std::string repo_name, std::string folder){
    cloneGit("https://github.com/", username, repo_name, folder);
}

extern std::vector<std::string> DependencyPathList;

void addDependencyToTempLinkableFiles(std::string dependency){
    std::string lib_path = DEFAULT_PACKAGE_PATH "/" + dependency + "/lib.a";
    DependencyPathList.push_back(lib_path);
}

extern std::vector<std::string> LibrariesList;

void downloadSubDependencies(std::string username, std::string repo_name){
    std::string root_build_toml_path = DEFAULT_PACKAGE_PATH "/" + repo_name + "/build.toml";
    if (!fs::exists(fs::path(root_build_toml_path))){
        return;
    }
    //std::cout << "DOWNLOAD SUB DEPENDENCIES" << std::endl;
    auto dependency_config = toml::parse_file(root_build_toml_path);
    auto github_dependencies = dependency_config["dependencies"]["github"];
    auto libraries =  dependency_config["build"]["libraries"];
    //addLibrariesToList(libraries);
    if (toml::array* arr = libraries.as_array()){
        arr->for_each([](auto&& dep){
            if constexpr (toml::is_string<decltype(dep)>){
                //std::cout << "add to LibrariesList : " << (std::string)dep << std::endl;
                LibrariesList.push_back((std::string)dep);
            }
        });
    }
    if (toml::array* arr = github_dependencies.as_array()){
        arr->for_each([](auto&& dep){
            if constexpr (toml::is_string<decltype(dep)>){
                std::string dependency = *dep;
                std::string username = dependency.substr(0, dependency.find('/'));
                std::string repo_name = dependency.substr(dependency.find('/')+1, dependency.size());
                cloneGithub(username, repo_name, DEFAULT_PACKAGE_PATH);
                downloadSubDependencies(username, repo_name);
                addDependencyToTempLinkableFiles(repo_name);
            }
        });
    }
}

void downloadDependencies(toml::v3::table config){
    auto github_dependencies = config["dependencies"]["github"];
    if (toml::array* arr = github_dependencies.as_array()){
        arr->for_each([](auto&& dep){
            if constexpr (toml::is_string<decltype(dep)>){
            std::string dependency = *dep;
            std::string username = dependency.substr(0, dependency.find('/'));
            std::string repo_name = dependency.substr(dependency.find('/')+1, dependency.size());
            cloneGithub(username, repo_name, DEFAULT_PACKAGE_PATH);
            downloadSubDependencies(username, repo_name);
            addDependencyToTempLinkableFiles(repo_name);
            }
        });
    }
}

extern std::string filename_config;

void addDependency(std::string dependency_name, toml::v3::table& config){
    auto github_dependencies = config["dependencies"]["github"];
    if (toml::array* arr = github_dependencies.as_array()){
        arr->for_each([dependency_name](auto&& dep){
            if constexpr (toml::is_string<decltype(dep)>){
                std::cout << "add dependency : " << dependency_name << std::endl; // TODO : remove this / make it debug output
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

void buildDependencies(toml::v3::table& config){
    auto github_dependencies = config["dependencies"]["github"];
    if (toml::array* arr = github_dependencies.as_array()){
        arr->for_each([](auto&& dep){
            if constexpr (toml::is_string<decltype(dep)>){
            std::string dependency = *dep;
            std::string username = dependency.substr(0, dependency.find('/'));
            std::string repo_name = dependency.substr(dependency.find('/')+1, dependency.size());
            std::string repo_path = DEFAULT_PACKAGE_PATH "/" + repo_name;
            buildDependency(repo_path);
            }
        });
    }
}