#include <string>
#include <memory>
#include <vector>

class ProgramReturn {
public:
    int exit_status;
    std::string buffer;
    ProgramReturn(int exit_status, const std::string& buffer) : exit_status(exit_status), buffer(buffer) {}
};

std::unique_ptr<ProgramReturn> runCommand(const std::string cmd);
void compileFile(std::string target, std::string arguments, std::string path, std::string sysroot, std::string out_path = "");
void linkFiles(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot, bool is_gc, bool is_strip_mode, std::string linker_path);
void linkLibrary(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot);
void linkDynamicLibrary(std::vector<std::string> PathList, std::string outfilename, std::string target, std::string args, std::string sysroot);
void openWebPage(std::string url);
void buildDependency(std::string path /*dependency*/);
bool doesExeExist(std::string filename);