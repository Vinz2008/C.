#include "../../src/config.h"
#include <toml++/toml.h>
#include <iostream>

int main(int argc, char** argv){
    std::string filename_config = "build.toml";
    for (int i = 0; i < argc; i++){
    std::string arg = argv[i];
    if (arg == "-f"){
    i++;
    filename_config = argv[i];
    }
    }
    auto config = toml::parse_file(filename_config);
    std::string_view project_name = config["project"]["name"].value_or("");
    std::cout << "project name : " << project_name << "\n";
}
