#include <string>

std::string get_pkg_config_linker_args(std::string library_name);
std::string get_llvm_config_linker_args();
std::string get_pkg_config_cflags_args(std::string library_name);
void handle_library_name(std::string library, std::string& linker_args);