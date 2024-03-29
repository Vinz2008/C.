#define  TOML_HEADER_ONLY 1
#include <toml++/toml.h>

void cleanFiles(toml::v3::table& config, std::string src_folder, std::string type);