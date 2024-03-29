#include "../../src/config.h"
#include "cli.h"
#include "files.h"
#include "dependencies.h"
#include "project_creator.h"
#include "install.h"
#include "tests.h"
#include "log.h"
#include <toml++/toml.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

void buildSubfolders(toml::v3::table& config, std::string_view type, std::string target, std::string sysroot);
void buildFolder(std::string src_folder, toml::v3::table& config, std::string_view type, std::string target, std::string sysroot, bool is_gc, int thread_number);

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
    INIT_MODE = -11,
    HELP_MODE = -12,
    TEST_MODE = -13,
};

std::string filename_config = "build.toml";
std::vector<std::string> PathList;

std::vector<std::string> DependencyPathList;

std::vector<std::string> LibrariesList;

bool is_cross_compiling = false;

bool src_folder_exists = true;

bool silent_mode = false;



// keep it in this file to not have multiple file with the tomlplusplus header included
bool shouldRebuildSTD(std::string std_path, std::string target, bool is_gc){
    std::string last_build_toml_path = std_path + "/last_build.toml";
    if (!fs::exists(fs::path(std_path + "/libstd.a"))){
        return true;
    }
    if (fs::exists(fs::path(last_build_toml_path))){ 
        auto last_build_toml = toml::parse_file(last_build_toml_path);
        if (!last_build_toml["target"].is_value()){
            return true;
        }
        std::string last_target = last_build_toml["target"].value_or("");
        /*if (last_target == ""){
            return true;
        }*/
        bool last_is_gc = last_build_toml["is_gc"].value_or(true);
        return (last_target != target || last_is_gc != is_gc);
    }
    return true;
}

bool isSubprojectGC(std::string path){
    //std::cout << "project gc path : " << path + "/build.toml" << std::endl;
    auto subproject_config = toml::parse_file(path + "/build.toml");
    bool is_gc = subproject_config["build"]["gc"].value_or(true);
    //std::cout << "project gc : " << is_gc << std::endl;
    return is_gc;
}

void writeLastBuildToml(std::string std_path, std::string target, bool is_gc){
    std::string last_build_toml_path = std_path + "/last_build.toml";
    fs::remove(fs::path(last_build_toml_path));
    std::ofstream LastBuildFstream(last_build_toml_path);
    LastBuildFstream << "target = \"" << target <<"\"\n";
    LastBuildFstream << "is_gc = " << ((is_gc) ? "true" : "false") << "\n"; 
    LastBuildFstream.close();
    //build_toml["is_gc"] = is_gc;
    //build_toml["target"] = target;
}

// TODO : maybe move some functions in other files/create new files

void addDependencyToTempLinkableFiles(std::string dependency){
    std::string lib_path = DEFAULT_PACKAGE_PATH "/" + dependency + "/lib.a";
    DependencyPathList.push_back(lib_path);
}

void addDependenciesToLinkableFiles(std::vector<std::string>& PathListPassed){
    for (int i = 0; i < DependencyPathList.size(); i++){
        PathListPassed.push_back(DependencyPathList.at(i));
    }
}

void addLibrariesToList(toml::v3::node_view<toml::v3::node>& libraries){
    if (toml::array* arr = libraries.as_array()){
        arr->for_each([](auto&& dep){
            if constexpr (toml::is_string<decltype(dep)>){
                //std::cout << "add to LibrariesList : " << (std::string)dep << std::endl;
                LibrariesList.push_back((std::string)dep);
            }
        });
    }
}

// also add dependencies libraries
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
                Log() << "script : " << cmd << "\n";
                std::unique_ptr<ProgramReturn> returnScript = runCommand(cmd);
                std::cout << returnScript->buffer << std::endl;
            }
        });
    }
}

void addCustomLinkableFiles(toml::v3::table& config, std::vector<std::string>& LinkPathList = PathList){
    auto linkableFiles = config["custom"]["linkablefiles"];
    if (toml::array* arr = linkableFiles.as_array()){
        arr->for_each([&LinkPathList](auto&& file){
            if constexpr (toml::is_string<decltype(file)>){
                LinkPathList.push_back((std::string) file);
            }
        });
    }
}

std::string get_pkg_config_linker_args(std::string library_name){
    std::string cmd = "pkg-config --libs " + library_name;
    auto cmd_out = runCommand(cmd);
    return cmd_out->buffer;
}

std::string get_llvm_config_linker_args(){
    std::string cmd = "llvm-config --ldflags --system-libs --libs core";
    auto cmd_out = runCommand(cmd);
    std::string ret = cmd_out->buffer;
    std::replace(ret.begin(), ret.end(), '\n', ' ');
    std::cout << "ret : " << ret << std::endl;
    return ret;
}

std::string get_pkg_config_cflags_args(std::string library_name){
    std::string cmd = "pkg-config --cflags " + library_name;
    auto cmd_out = runCommand(cmd);
    return cmd_out->buffer;
}

void handle_library_name(std::string library, std::string& linker_args){
    if (library == "pthread"){
        linker_args += "-lpthread ";
    } else if (library == "gtk"){
        linker_args += get_pkg_config_linker_args("gtk4");
    } else if (library == "raylib" || library == "lua"){
        linker_args += get_pkg_config_linker_args((std::string)library);
    } else if (library == "llvm"){
        linker_args += get_llvm_config_linker_args();
    } else if (library == "clang"){
        linker_args += "-lclang";
    } else {
        std::cout << "Warning : unknown library : " << library << std::endl;
        linker_args += get_pkg_config_linker_args((std::string)library);
    }
}

std::string get_libraries_linker_args(toml::v3::table&  config){
    std::string linker_args = "";
    auto libraries = config["build"]["libraries"];
    bool found_at_least_one = false;
    if (toml::array* arr = libraries.as_array()){
        arr->for_each([&linker_args, &found_at_least_one](auto&& library){
            if constexpr (toml::is_string<decltype(library)>){
                found_at_least_one = true;
                /*if (library == "pthread"){
                    linker_args += "-lpthread ";
                } else if (library == "gtk"){
                    linker_args += get_pkg_config_linker_args("gtk4");
                } else if (library == "raylib" || library == "lua"){
                    linker_args += get_pkg_config_linker_args((std::string)library);
                } else if (library == "llvm"){
                    linker_args += get_llvm_config_linker_args();
                } else {
                    std::cout << "Warning : unknown library : " << library << std::endl;
                    linker_args += get_pkg_config_linker_args((std::string)library);
                }*/
                handle_library_name((std::string)library, linker_args);
            }
        });
    }
    //std::cout << "LibrariesList.size() " << LibrariesList.size() << std::endl;
    if (!found_at_least_one && LibrariesList.size() == 0){
        return "";
    }
    for (int i = 0; i < LibrariesList.size(); i++){
        //linker_args LibrariesList.at(i);
        std::cout << "adding lib " << LibrariesList.at(i) << std::endl;
        handle_library_name(LibrariesList.at(i), linker_args);
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
                Log() << "cmd : " << cmd << "\n";
                std::unique_ptr<ProgramReturn> returnCmd = runCommand((std::string) cmd);
                Log() << returnCmd->buffer << "\n";
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

void printHelp(){
    std::cout << "Usage : cpoint-build [options] [command]" << std::endl;
    std::cout << "Options : " << std::endl;
    std::cout << "    -f : Select the toml config file for the project" << std::endl;
    std::cout << "    -j : Select the number of threads to use (use it like in Make, ex : \"-j8\")" << std::endl;
    std::cout << "    -h : Print this page (You wan also use --help)" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands : " << std::endl;
    std::cout << "    build : Build the project in the folder" << std::endl;
    std::cout << "    clean : Clean the project" << std::endl;
    std::cout << "    info : Print infos of the project" << std::endl;
    std::cout << "    download/update : Update dependencies" << std::endl;
    std::cout << "    add : Add a dependency to the project" << std::endl;
    std::cout << "    new : Create a new project in a new folder" << std::endl;
    std::cout << "    open-page : Open the website of the project" << std::endl;
    std::cout << "    init : Init a project in the folder" << std::endl;
    std::cout << "    cross-compile : Cross-compile the project" << std::endl;
    std::cout << "    install : Install the binary" << std::endl;
    std::cout << "    install_path : Print the path where binaries are installed" << std::endl;
}

int main(int argc, char** argv){

    std::vector<std::string> needed_exes = {"clang"};
    for (int i = 0; i < needed_exes.size(); i++){
        if (!doesExeExist(needed_exes.at(i))){
            std::cout << "Couldn't find the " << needed_exes.at(i) << " exe" << std::endl;
            exit(1);
        }
    }
    enum mode modeBuild = BUILD_MODE;
    std::string src_folder = "src/";
    std::string dependency_to_add = "";
    std::string binary_to_install = "";
    std::string project_name_to_create = "";
    std::string install_local_project_path = "";
    bool is_gc = true;
    int thread_number = 1;
    for (int i = 0; i < argc; i++){
    std::string arg = argv[i];
    if (arg == "-f"){
        i++;
        filename_config = argv[i];
    } else if (arg.rfind("-j", 0) == 0){
        thread_number = std::stoi(arg.substr(2, arg.length() - 2));

        //i++;
        //thread_number = std::stoi((std::string)argv[i]);
    } else if (arg == "--silent"){
        silent_mode = true;
    } else if (arg == "-C"){
        int pos_arg = i;
        char** argv_new = new char*[argc-2];
        int pos_argv = 0;
        for (int i = 0; i < argc; i++){
            if (i != pos_arg && i != pos_arg + 1){
                argv_new[pos_argv] = argv[i];
                pos_argv++;
            }
        }
        /*for (int i = 0; i < argc-2; i++){
            std::cout << "argv_new[" << i << "] : " << std::string(argv_new[i]) << std::endl;
        }
        std::cout << "argv[pos_arg+1] : " << std::string(argv[pos_arg+1]) << std::endl;*/
        fs::current_path(argv[pos_arg+1]);
        int returned_val = main(argc-2, argv_new);
        delete[] argv_new;
        return returned_val;
    } else if (arg == "build"){
        modeBuild = BUILD_MODE;
    } else if (arg == "clean"){
        modeBuild = CLEAN_MODE;
    } else if (arg == "test"){
        modeBuild = TEST_MODE;
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
        if (i >= argc){
            std::cout << "Missing folder argument when creating a project" << std::endl;
            exit(1);
        }
        project_name_to_create = argv[i];
    } else if (arg == "open-page"){
        modeBuild = OPEN_PAGE_MODE;
        //i++;
    } else if (arg == "init"){
        modeBuild = INIT_MODE;
    } else if (arg == "cross-compile"){
        modeBuild = CROSS_COMPILE_MODE;
        is_cross_compiling = true;
        //i++;
    } else if (arg == "install"){
        modeBuild = INSTALL_MODE;
        i++;
        if ((std::string)argv[i] == "--path"){
        i++;
        install_local_project_path = argv[i];
        } else {
        binary_to_install = argv[i];
        }
    } else if (arg == "install_path"){
        modeBuild = INSTALL_PATH_MODE;
    } else if (arg == "-h" || arg == "--help"){
        modeBuild = HELP_MODE;
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
    if (modeBuild == INIT_MODE){
        initProject("exe", "./");
        return 0;
    }
    if (modeBuild == HELP_MODE){
        printHelp();
        return 0;
    }
    if (modeBuild == INSTALL_MODE){
        std::string repo_path = "";
        std::string username = "";
        std::string repo_name = "";
        std::string package_name = "";
        if (install_local_project_path != ""){
            repo_path = install_local_project_path;
        } else {
            username = binary_to_install.substr(0, binary_to_install.find('/'));
            repo_name = binary_to_install.substr(binary_to_install.find('/')+1, binary_to_install.size());
            cloneGithub(username, repo_name, DEFAULT_PACKAGE_PATH);
            repo_path = DEFAULT_PACKAGE_PATH "/" + repo_name;
            package_name = repo_name;
        }
        buildDependency(repo_path);
        auto config = toml::parse_file(repo_path + "/build.toml");
        std::string outfile = (std::string)config["build"]["outfile"].value_or("");
        if (outfile == ""){
            outfile = "a.out";
        }
        std::string install_outfile = outfile;
        if (install_outfile == "a.out" && repo_name != ""){
            install_outfile = repo_name;
        } else {
            install_outfile = config["project"]["name"].value_or("");
            package_name = install_outfile;         
        }
        if (fs::exists((fs::path){ repo_path + "/" + outfile })){
        installBinary(repo_path + "/" + outfile, install_outfile);
        }
        std::string username_or_nothing = (username != "") ? username + "/" : (std::string)"";
        std::cout << "Installed successfully the " << username_or_nothing << /*repo_name*/ package_name << " repo to " << DEFAULT_BUILD_INSTALL_PATH "/" + install_outfile << "\n";
        auto subprojects = config["subfolders"]["projects"];
        if (toml::array* arr = subprojects.as_array()){
        arr->for_each([&config, repo_path, username, repo_name](auto&& sub){
            if constexpr (toml::is_string<decltype(sub)>){
                //std::cout << "sub : " << sub << std::endl;
                //buildSubproject((std::string)sub);
                std::string sub_path = repo_path + "/" + (std::string)sub;
                auto sub_config = toml::parse_file(sub_path + "/build.toml");
                std::string sub_outfile = (std::string)sub_config["build"]["outfile"].value_or("");
                std::string install_sub_outfile = sub_outfile;
                if (install_sub_outfile == "a.out"){
                    install_sub_outfile = (std::string)sub;
                }
                installBinary(sub_path + "/" + sub_outfile, install_sub_outfile);
                std::string username_or_nothing = (username != "") ? username + "/" : (std::string)"";
                std::cout << "Installed successfully the " << username_or_nothing << repo_name << "repo " << sub << " binary " << " to " << DEFAULT_BUILD_INSTALL_PATH "/" + install_sub_outfile << "\n";
            }
        });
    }
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
    is_gc = config["build"]["gc"].value_or(true);
    bool is_strip_mode = config["build"]["strip"].value_or(false);
    std::string linker_path = (std::string)config["build"]["linker_path"].value_or("");
    if (src_folder_temp != ""){
        src_folder = src_folder_temp;
    }
    if (!fs::is_directory(src_folder)){
        src_folder_exists = false;
    }
    PathList.clear();
    if (modeBuild == CLEAN_MODE){
        cleanFiles(config, src_folder, (std::string)type);
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
        if (linker_path != ""){
            if (!doesExeExist(linker_path)){
                std::cout << "linker specified in build.toml : " << linker_path << " doesn't exist" << "\n";
                exit(1);
            }
        }
        if (modeBuild == CROSS_COMPILE_MODE){
        linker_args += cross_compile_linker_args;
        target = target_cross;
        sysroot = sysroot_cross;
        }
        if (modeBuild != CROSS_COMPILE_MODE){
            downloadDependencies(config);
            buildDependencies(config);
        }
        std::string c_libraries_linker_args = get_libraries_linker_args(config);
        runPrebuildCommands(config);
        buildSubfolders(config, type, target, sysroot, is_gc, thread_number);
        if (src_folder_exists){
        buildFolder(src_folder, config, type, target, sysroot, is_gc, thread_number);
        }
        buildSubprojects(config);
        runCustomScripts(config);
        addCustomLinkableFiles(config);
        if (modeBuild != CROSS_COMPILE_MODE){
            addDependenciesToLinkableFiles(PathList);
        }
        linker_args += c_libraries_linker_args;
        if (src_folder_exists){
        if (type == "exe"){
            linkFiles(PathList, outfilename, target, linker_args, sysroot, is_gc, is_strip_mode, linker_path);
        } else if (type == "library"){
            linkLibrary(PathList, outfilename, target, linker_args, sysroot);
        } else if (type == "dynlib"){
            linkDynamicLibrary(PathList, outfilename, target, linker_args, sysroot);
        }
        }
    } else if (modeBuild == TEST_MODE){
        runPrebuildCommands(config);
        // TODO : add also subfolders
        std::vector<std::string> LinkPathList;
        downloadDependencies(config);
        buildDependencies(config);
        addCustomLinkableFiles(config, LinkPathList);
        addDependenciesToLinkableFiles(LinkPathList);
        std::vector<std::string> TestPathList = getFilenamesWithExtension(".cpoint", src_folder);
        buildTests(TestPathList, LinkPathList);
        runTests(TestPathList);
    }
}
