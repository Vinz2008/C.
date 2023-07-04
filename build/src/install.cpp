#include <iostream>
#include <filesystem>
#include "../../src/config.h"

namespace fs = std::filesystem;

#ifdef _WIN32
#define INSTALL_PATH_ACTION  "run this command "
#define CMD_INSTALL_PATH_WITHOUT_PATH   "setx -m PATH \""
#define PREFIX_AFTER_CMD ""
#else 
#define INSTALL_PATH_ACTION  "add this command to your .bashrc or your .zshrc "
#define CMD_INSTALL_PATH_WITHOUT_PATH   "export PATH=\""
#define PREFIX_AFTER_CMD ":$PATH"
#endif

void installBinary(std::string exe_path, std::string name_binary){
    std::string out_path = DEFAULT_BUILD_INSTALL_PATH "/";
    if (name_binary == ""){
     out_path += fs::path(exe_path).filename();
    } else {
        out_path += name_binary;
    }
    try {
        fs::remove(out_path);
        fs::copy_file(exe_path, out_path);
    } catch (fs::filesystem_error& e) {
        std::cout << "Could not install binary" << e.what() << "\n";
        exit(1);
    }
}

void printInstallPathMessage(){
    std::cout << "You need to " << INSTALL_PATH_ACTION << "to add binaries installed with cpoint-build to your path : \n " << CMD_INSTALL_PATH_WITHOUT_PATH << DEFAULT_BUILD_INSTALL_PATH << PREFIX_AFTER_CMD << "\"" << std::endl;
}