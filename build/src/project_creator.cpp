#include <string>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

std::string file_main_template = R"(import @std/print.cpoint

func main(){
    printstr("Hello World !\n")
}
)";


std::string gitignore_template = R"(*.out
*.a
*.ll
*.o
*.log
*.temp
)";

// need to add type variable
std::string build_toml_template = R"([project]
name = "test"
homepage = "https://github.com/test"
license = "GPL"
)";

void initProject(std::string type, std::string folder){
    fs::create_directories(folder);
    std::string code_folder = folder + "src/";
    fs::create_directories(code_folder);
    std::ofstream config_file(folder + "build.toml");
    std::ofstream gitignore_file(folder + ".gitignore");
    std::ofstream main_file(code_folder + "main.cpoint");
    main_file << file_main_template;
    gitignore_file << gitignore_template;
    config_file << build_toml_template;
    config_file << "type = \"" << type << "\"" << std::endl;
    config_file.close();
    main_file.close();
    gitignore_file.close();
}


void createProject(std::string type, std::string project_name){
    std::string folder = "./" + project_name + "/";
    initProject(type, folder);
}