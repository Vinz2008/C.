#include <iostream>
#include <memory>
#include <cstdarg>



//#pragma once

#ifndef ERRORS_H
#define ERRORS_H


struct Source_location {
    int line_nb;
    int col_nb;
    bool is_empty;
    std::string line;
};


std::ostream& operator<<(std::ostream& os, struct Source_location Loc);

class Compiler_context{
public:
    std::string filename;
    struct Source_location lexloc;
    std::string line;
    struct Source_location curloc;
    Compiler_context(const std::string &filename, int line_nb, int col_nb, const std::string &line) : filename(filename), lexloc({line_nb, col_nb}), line(line), curloc({line_nb, col_nb}) {}
};

void logErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, ...);

#include "ast.h"

void vlogErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, std::va_list args, Source_location astLoc);
int stringDistance(std::string s1, std::string s2);


#endif