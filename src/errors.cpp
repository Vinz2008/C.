#include "errors.h"
#include "color.h"
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>

extern bool errors_found;
extern int error_count;

std::ostream& operator<<(std::ostream& os, struct Source_location Loc){
    os << Loc.line_nb << ":" << Loc.col_nb;
    return os;
}

bool operator!=(Source_location loc1, Source_location loc2){
    return !((loc1.col_nb == loc2.col_nb) && (loc1.is_empty == loc2.is_empty) && (loc1.line == loc2.line) && (loc1.line_nb == loc2.line_nb));
}

// TODO : remove this (not used ?)
int stringDistance(std::string s1, std::string s2) {
    // Create a table to store the results of subproblems
    std::vector<std::vector<int>> dp(s1.length() + 1, std::vector<int>(s2.length() + 1));
 
    // Initialize the table
    for (int i = 0; i <= s1.length(); i++) {
        dp[i][0] = i;
    }
    for (int j = 0; j <= s2.length(); j++) {
        dp[0][j] = j;
    }
 
    // Populate the table using dynamic programming
    for (int i = 1; i <= s1.length(); i++) {
        for (int j = 1; j <= s2.length(); j++) {
            if (s1[i-1] == s2[j-1]) {
                dp[i][j] = dp[i-1][j-1];
            } else {
                dp[i][j] = 1 + std::min(dp[i-1][j], std::min(dp[i][j-1], dp[i-1][j-1]));
            }
        }
    }
 
    // Return the edit distance
    return dp[s1.length()][s2.length()];
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