#include <string>

#define  TOML_HEADER_ONLY 1
#include <toml++/toml.h>

void cloneGithub(std::string username, std::string repo_name, std::string folder);
void addDependency(std::string dependency_name, toml::v3::table& config);
void downloadDependencies(toml::v3::table config);
void buildDependencies(toml::v3::table& config);