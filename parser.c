#include "parser.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#ifdef __linux__
#include <linux/limits.h>
#endif
#include "lexer.h"


int parser(char line[40],struct ParsedText* parsedText, int posFirstQuote, int posLastQuote, int posFirstParenthesis, int posLastParenthesis,int debugMode) {
	int c = 0;
    int i, i2;
    int isFunctionInt = 0;
    memset(parsedText->lineList, 0, sizeof(parsedText->lineList));
    char tempStr[PATH_MAX];
    char* libraryName;
    char *pch = strtok(line," ");
    while (pch != NULL){
    ++parsedText->sizeLineList;
    if (debugMode == 1) {
    printf("sizeLineList : %d\n", parsedText->sizeLineList);
    printf ("pch : %s\n",pch);
    }
    parsedText->lineList[c] = malloc(10 * sizeof(char));
	memset(parsedText->lineList[c], 0 ,sizeof(parsedText->lineList[c]));
    parsedText->lineList[c] = pch;
    for (i = 0; i < strlen(parsedText->lineList[c]); i++) {
        if (debugMode == 1) {
        printf("lineList[c] length  : %lu\n", strlen(parsedText->lineList[c]));
        printf("char %i : %c\n",i,parsedText->lineList[c][i]);
        }
        if (parsedText->lineList[c][i] == '(') {
            posFirstParenthesis = i;
            if (debugMode == 1) {
            printf("posFirstParenthesis: %i\n",posFirstParenthesis);
            }
            for (i2 = i; i2 < strlen(parsedText->lineList[c]); i2++ ){
                if (parsedText->lineList[c][i2] == ")"[0]) {
                posLastParenthesis = i2;
                if (debugMode == 1) {
                printf("posLastParenthesis: %i\n",posLastParenthesis);
                }
	            break;
                }
            }
        }
	    else if (parsedText->lineList[c][i] == "\""[0]) {
	    if (posLastQuote == i) {
        if (debugMode == 1) {
	    printf("end quote so pass it\n");
        }
	    }
	    else {
	    posFirstQuote = i;
        if (debugMode == 1) {
	    printf("posFirstQuote: %i\n", posFirstQuote);
        }
	    for (i2 = i+ 1; i2 < strlen(parsedText->lineList[c]); i2++) {
	    if (parsedText->lineList[c][i2] == "\""[0]) {
	    posLastQuote = i2;
        if (debugMode == 1) {
	    printf("posLastQuote: %i\n", posLastQuote);
        }
	    break;
	    }
	    }
	    }
	    }

    }
    //pch = strtok (NULL, " \t");
	pch = strtok (NULL, " ");
	c++;
	}
	return 0;
}
