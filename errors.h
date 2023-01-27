#include <iostream>

class Compiler_context{
public:
    int line_nb;
    std::string line;
    Compiler_context(int line_nb, const std::string &line) : line_nb(line_nb), line(line) {}
};

void logErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, ...);
