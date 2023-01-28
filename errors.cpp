#include "errors.h"
#include "color.h"
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <memory>

void logErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, ...){
   std::va_list args;
   va_start(args, format);
   fprintf(stderr, RED "Error in line %d\n" CRESET, cc->line_nb);
   vfprintf(stderr, format, args);
   fprintf(stderr, "\n\t%s\n", cc->line.c_str());
   va_end(args);
   exit(1);
}
