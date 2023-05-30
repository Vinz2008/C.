#include "errors.h"
#include "color.h"
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <memory>

extern bool errors_found;
extern int error_count;



void vlogErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, std::va_list args){
   if (cc == NULL){
      fprintf(stderr, "cc NULL\n");
   }
   fprintf(stderr, RED "Error in line %d:%d\n" CRESET, cc->loc.line_nb, cc->loc.col_nb > 0 ? cc->loc.col_nb-1 : cc->loc.col_nb);
   vfprintf(stderr, format, args);
   fprintf(stderr, "\n\t%s\n", cc->line.c_str());
   fprintf(stderr, "\t");
   for (int i = 0; i < cc->loc.col_nb-2; i++){
   fprintf(stderr, " ");
   }
   fprintf(stderr, "^\n");
   errors_found = true;
   error_count++;
   //exit(1);
}


void logErrorExit(std::unique_ptr<Compiler_context> cc, const char* format, ...){
   std::va_list args;
   va_start(args, format);
   vlogErrorExit(std::move(cc), format, args);
   va_end(args);
}