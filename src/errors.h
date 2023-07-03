#include <iostream>
#include <memory>
#include <cstdarg>

#pragma once

struct Source_location {
    int line_nb;
    int col_nb;
};

class Compiler_context{
public:
    std::string filename;
    struct Source_location loc;
    std::string line;
    Compiler_context(const std::string &filename, int line_nb, int col_nb, const std::string &line) : filename(filename), loc({line_nb, col_nb}), line(line) {}
};

void logErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, ...);
void vlogErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, std::va_list args);
