#include "errors.h"
#include "color.h"
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <memory>

void logErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, ...){
   std::va_list args;
   va_start(args, format);
   fprintf(stderr, RED "Error in line %d:%d\n" CRESET, cc->line_nb, cc->col_nb-1);
   vfprintf(stderr, format, args);
   fprintf(stderr, "\n\t%s\n", cc->line.c_str());
   fprintf(stderr, "\t");
   for (int i = 0; i < cc->col_nb-2; i++){
   fprintf(stderr, " ");
   }
   fprintf(stderr, "^\n");
   va_end(args);
   exit(1);
}
