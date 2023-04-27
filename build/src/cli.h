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
void compileFile(std::string target, std::string arguments, std::string path);
void linkFiles(std::vector<std::string> PathList);
void linkLibrary(std::vector<std::string> PathList);
void linkDynamicLibrary(std::vector<std::string> PathList);