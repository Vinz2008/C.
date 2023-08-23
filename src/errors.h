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
bool operator!=(Source_location loc1, Source_location loc2);

class Compiler_context{
public:
    std::string filename;
    struct Source_location lexloc;
    std::string line;
    struct Source_location curloc;
    bool std_mode;
    bool gc_mode;
    bool debug_mode;
    bool is_release_mode;
    bool test_mode;
    Compiler_context(const std::string &filename, int line_nb, int col_nb, const std::string &line, bool std_mode = true, bool gc_mode = true, bool debug_mode = false, bool is_release_mode = false, bool test_mode = false) 
                    : filename(filename), lexloc({line_nb, col_nb}), line(line), curloc({line_nb, col_nb}), std_mode(std_mode), gc_mode(gc_mode), debug_mode(debug_mode), is_release_mode(is_release_mode), test_mode(test_mode) {}
};

void logErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, ...);

#include "ast.h"

void vlogErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, std::va_list args, Source_location astLoc);
int stringDistance(std::string s1, std::string s2);


#endif