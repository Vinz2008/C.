#include <vector>
#include <string>

void linkFiles(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot, bool is_gc, bool is_strip_mode, std::string linker_path, bool should_use_internal_lld);
void linkLibrary(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot);
void linkDynamicLibrary(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot);