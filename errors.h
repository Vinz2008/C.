#include <iostream>
#include <memory>

class Compiler_context{
public:
    int line_nb;
    int col_nb;
    std::string line;
    std::string filename;
    Compiler_context(const std::string &filename, int line_nb, int col_nb, const std::string &line) : filename(filename), line_nb(line_nb), col_nb(col_nb), line(line) {}
};

void logErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, ...);
