#include "cli.h"
#include <iostream>
#include <memory>
#include <array>

#ifdef _WIN32
#include "windows.h"
#endif

ProgramReturn runCommand(const std::string cmd){
    int exit_status = 0;
    auto pipe = popen(cmd.c_str(), "r");
    if (pipe == nullptr){
        std::cout << "ERROR : Cannot open pipe" << std::endl;
        exit(1);
    }
    std::array<char, 256> buffer;
    std::string result;

    while(not std::feof(pipe))
    {
        auto bytes = std::fread(buffer.data(), 1, buffer.size(), pipe);
        result.append(buffer.data(), bytes);
    }
    auto rc = pclose(pipe);
    if (WEXITSTATUS(rc)){
        exit_status = WEXITSTATUS(rc);
    }
    return ProgramReturn(exit_status, result);

}