#include "lexer.h"

struct ParsedText {
    char** lineList;
    enum Token tag;
    int sizeLineList;
};

int parser(char line[40],struct ParsedText* parsedText, int posFirstQuote, int posLastQuote, int posFirstParenthesis, int posLastParenthesis,int debugMode);
