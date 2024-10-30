#include "errors.h"
#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include "color.h"

extern bool errors_found;
extern int error_count;

std::ostream& operator<<(std::ostream& os, struct Source_location Loc){
    os << Loc.line_nb << ":" << Loc.col_nb;
    return os;
}

bool operator!=(Source_location loc1, Source_location loc2){
    return !((loc1.col_nb == loc2.col_nb) && (loc1.is_empty == loc2.is_empty) && (loc1.line == loc2.line) && (loc1.line_nb == loc2.line_nb));
}

void vlogErrorExit(Source_location cc_lexloc, std::string line, std::string filename, const char* format, std::va_list args, Source_location astLoc){
    Source_location loc;
    if (!astLoc.is_empty){
        loc = astLoc;
        //loc.line = cc.line; // TODO : remove this and fix line printing. Remove line from Comp_context and put it in source_location ?
    } else {
        loc = cc_lexloc;
    }
   fprintf(stderr, RED "Error in %s:%d:%d\n" CRESET, filename.c_str(), loc.line_nb, loc.col_nb > 0 ? loc.col_nb-1 : loc.col_nb);
   vfprintf(stderr, format, args);
   fprintf(stderr, "\n\t%s\n", loc.line.c_str());
   fprintf(stderr, "\t");
   for (int i = 0; i < loc.col_nb-2; i++){
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
   assert(cc != nullptr);
   vlogErrorExit(cc->lexloc, cc->line, cc->filename, format, args, {});
   va_end(args);
}