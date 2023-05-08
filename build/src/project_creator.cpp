#include <string>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

std::string file_main_template = R"(import @std/print.cpoint

func main(){
    printstr("Hello World !\n")
}
)";


// need to add type variable
std::string build_toml_template = R"([project]
name = "test"
homepage = "https://github.com/test"
license = "GPL"
)";

void createProject(std::string type, std::string project_name){
    std::string folder = "./" + project_name + "/";
    fs::create_directories(folder);
    std::string code_folder = folder + "src/";
    fs::create_directories(code_folder);
    std::ofstream config_file(folder + "build.toml");
    std::ofstream main_file(code_folder + "main.cpoint");
    main_file << file_main_template;
    config_file << build_toml_template;
    config_file << "type = \"" << type << "\"" << std::endl;
    config_file.close();
    main_file.close();
}