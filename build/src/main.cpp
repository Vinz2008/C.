#include "../../src/config.h"
#include "cli.h"
#include "files.h"
#include "dependencies.h"
#include "project_creator.h"
#include "install.h"
#include <toml++/toml.h>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

void buildSubfolders(toml::v3::table& config, std::string_view type, std::string target, std::string sysroot);
void buildFolder(std::string src_folder, toml::v3::table& config,std::string_view type, std::string target, std::string sysroot, bool is_gc);

enum mode {
    BUILD_MODE = -1,
    CLEAN_MODE = -2,
    INFO_MODE = -3,
    DOWNLOAD_MODE = -4,
    ADD_DEPENDENCY_MODE = -5,
    OPEN_PAGE_MODE = -6,
    NEW_PROJECT_MODE = -7,
    CROSS_COMPILE_MODE = -8,
    INSTALL_MODE = -9,
    INSTALL_PATH_MODE = -10,
};

std::string filename_config = "build.toml";
std::vector<std::string> PathList;

std::vector<std::string> DependencyPathList;

bool is_cross_compiling = false;

void addDependencyToTempLinkableFiles(std::string dependency){
    std::string lib_path = DEFAULT_PACKAGE_PATH "/" + dependency + "/lib.a";
    DependencyPathList.push_back(lib_path);
}

void addDependenciesToLinkableFiles(){
    for (int i = 0; i < DependencyPathList.size(); i++){
        PathList.push_back(DependencyPathList.at(i));
    }
}

void downloadSubDependencies(std::string username, std::string repo_name){
    std::string root_build_toml_path = DEFAULT_PACKAGE_PATH "/" + repo_name + "/build.toml";
    if (!fs::exists(fs::path(root_build_toml_path))){
        return;
    }
    auto dependency_config = toml::parse_file(root_build_toml_path);
    auto github_dependencies = dependency_config["dependencies"]["github"];
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

void buildSubfolders(toml::v3::table& config, std::string_view type, std::string target, std::string sysroot, bool is_gc){
    auto subfolders = config["subfolders"]["folders"];
    if (toml::array* arr = subfolders.as_array()){
        arr->for_each([&config, type, target, sysroot, is_gc](auto&& sub){
            if constexpr (toml::is_string<decltype(sub)>){
                std::cout << "sub : " << sub << std::endl;
                buildFolder((std::string)sub, config, type, target, sysroot, is_gc);
            }
        });
    }
}

void buildFolder(std::string src_folder, toml::v3::table& config, std::string_view type, std::string target, std::string sysroot, bool is_gc){
    std::string_view arguments_view = config["build"]["arguments"].value_or("");
    std::string arguments = (std::string) arguments_view;
    if (type == "dynlib"){
        arguments += " -fPIC ";
    }
    std::vector<std::string> localPathList = getFilenamesWithExtension(".cpoint", src_folder);
    PathList.insert(PathList.end(), localPathList.begin(), localPathList.end());
    for (auto const& path : localPathList){
        std::cout << path << ' ';
        if (!is_gc){
            arguments += " -no-gc ";
        }
        compileFile(target, /*"-no-gc" +*/ arguments, path, sysroot);
    }
    std::cout << std::endl;
}

void buildDependencies(toml::v3::table& config){
    auto github_dependencies = config["dependencies"]["github"];
    if (toml::array* arr = github_dependencies.as_array()){
        arr->for_each([](auto&& dep){
            if constexpr (toml::is_string<decltype(dep)>){
            std::string dependency = *dep;
            std::string username = dependency.substr(0, dependency.find('/'));
            std::string repo_name = dependency.substr(dependency.find('/')+1, dependency.size());
            buildDependency(repo_name);
            }
        });
    }
}

void runCustomScripts(toml::v3::table& config){
    auto scripts = config["custom"]["scripts"];
    if (toml::array* arr = scripts.as_array()){
        arr->for_each([config](auto&& script){
            if constexpr (toml::is_string<decltype(script)>){
                std::string cmd = (std::string)script;
                if (is_cross_compiling){
                std::cout << "cross-compiling : " << std::endl;
                std::string target_cross_compiler = config["cross-compile"]["target"].value_or("");
                cmd = "TARGET=" + target_cross_compiler + " " + (std::string)script;
                }
                std::cout << "script : " << cmd << std::endl;
                std::unique_ptr<ProgramReturn> returnScript = runCommand(cmd);
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

std::string get_pkg_config_linker_args(std::string library_name){
    std::string cmd = "pkg-config --libs " + library_name;
    auto cmd_out = runCommand(cmd);
    return cmd_out->buffer;
}

std::string get_libraries_linker_args(toml::v3::table&  config){
    std::string linker_args = "";
    auto libraries = config["build"]["libraries"];
    bool found_at_least_one = false;
    if (toml::array* arr = libraries.as_array()){
        arr->for_each([&linker_args, &found_at_least_one](auto&& library){
            if constexpr (toml::is_string<decltype(library)>){
                found_at_least_one = true;
                if (library == "pthread"){
                    linker_args += "-lpthread ";
                } else if (library == "gtk"){
                    linker_args += get_pkg_config_linker_args("gtk4");
                } else {
                    std::cout << "Warning : unknown library : " << library << std::endl;
                }
            }
        });
    }
    if (!found_at_least_one){
        return "";
    }
    if (linker_args.back() == '\n'){
        linker_args.resize(linker_args.size()-1);
    }
    return linker_args;
}


void runPrebuildCommands(toml::v3::table& config){
    auto commands = config["custom"]["prebuild_commands"];
    if (toml::array* arr = commands.as_array()){
        arr->for_each([config](auto&& cmd){
            if constexpr (toml::is_string<decltype(cmd)>){
                if (is_cross_compiling){
                std::string target_cross_compiler = config["cross-compile"]["target"].value_or("");
                cmd = "TARGET=" + target_cross_compiler + " " + (std::string)cmd;
                }
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
    std::string binary_to_install = "";
    std::string project_name_to_create = "";
    bool is_gc = true;
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
        //i++;
    } else if (arg == "cross-compile"){
        modeBuild = CROSS_COMPILE_MODE;
        is_cross_compiling = true;
        //i++;
    } else if (arg == "install"){
        modeBuild = INSTALL_MODE;
        i++;
        binary_to_install = argv[i];
    } else if (arg == "install_path"){
        modeBuild = INSTALL_PATH_MODE;
    }
    }
    if (modeBuild == INSTALL_PATH_MODE){
        printInstallPathMessage();
        return 0;
    }
    // instruction that don't need the toml file
    if (modeBuild == NEW_PROJECT_MODE){
        createProject("exe", project_name_to_create);
        return 0;
    }
    if (modeBuild == INSTALL_MODE){
        std::string username = binary_to_install.substr(0, binary_to_install.find('/'));
        std::string repo_name = binary_to_install.substr(binary_to_install.find('/')+1, binary_to_install.size());
        cloneGithub(username, repo_name, DEFAULT_PACKAGE_PATH);
        std::string repo_path = DEFAULT_PACKAGE_PATH "/" + repo_name;
        buildDependency(repo_name);
        auto config = toml::parse_file(repo_path + "/build.toml");
        std::string outfile = (std::string)config["build"]["outfile"].value_or("");
        if (outfile == ""){
            outfile = "a.out";
        }
        std::string install_outfile = outfile;
        if (install_outfile == "a.out"){
            install_outfile = repo_name;
        }
        installBinary(repo_path + "/" + outfile, install_outfile);
        std::cout << "Installed successfully the " << username << "/" << repo_name << " repo to " << DEFAULT_BUILD_INSTALL_PATH "/" + install_outfile << "\n";
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
    std::string c_libraries_linker_args = get_libraries_linker_args(config);
    is_gc = config["build"]["gc"].value_or(true);
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
            buildDependencies(config);
        }
        runPrebuildCommands(config);
        buildSubfolders(config, type, target, sysroot, is_gc);
        buildFolder(src_folder, config, type, target, sysroot, is_gc);
        runCustomScripts(config);
        addCustomLinkableFiles(config);
        if (modeBuild != CROSS_COMPILE_MODE){
            addDependenciesToLinkableFiles();
        }
        linker_args += c_libraries_linker_args;
        if (type == "exe"){
        linkFiles(PathList, outfilename, target, linker_args, sysroot, is_gc);
        } else if (type == "library"){
        linkLibrary(PathList, outfilename, target, linker_args, sysroot);
        } else if (type == "dynlib"){
            linkDynamicLibrary(PathList, outfilename, target, linker_args, sysroot);
        }
    }
}
