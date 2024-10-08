#include <string>
#include <memory>

class ProgramReturn {
public:
    int exit_status;
    std::string buffer;
    ProgramReturn(int exit_status, const std::string& buffer) : exit_status(exit_status), buffer(buffer) {}
};

ProgramReturn runCommand(const std::string cmd);