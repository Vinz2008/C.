#include <string>

#define  TOML_HEADER_ONLY 1
#include <toml++/toml.h>

void buildSubprojects(toml::v3::table& config);
void buildFolder(std::string src_folder, toml::v3::table& config, std::string_view type, std::string target, std::string sysroot, bool is_gc, int thread_number);
void buildSubfolders(toml::v3::table& config, std::string_view type, std::string target, std::string sysroot, bool is_gc, int thread_number);