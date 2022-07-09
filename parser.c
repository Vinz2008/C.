// FOR NOW NOT USED 
// I DON'T KNOW WHY , BUT THE LINELIST RETURNED IS EMPTY

#include "parser.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#ifdef __linux__
#include <linux/limits.h>
#endif

int parser(char line[40],char** lineList, int* sizeLineList, int posFirstQuote, int posLastQuote, int posFirstParenthesis, int posLastParenthesis,int debugMode) {
	int c = 0;
    int i, i2;
    int isFunctionInt = 0;
    memset(lineList, 0, sizeof(lineList));
    char tempStr[PATH_MAX];
    char* libraryName;
    char *pch = strtok(line," ");
    while (pch != NULL)
	{
    ++*sizeLineList;
    if (debugMode == 1) {
    printf("sizeLineList : %d\n", *sizeLineList);
    printf ("pch : %s\n",pch);
    }
    lineList[c] = malloc(10 * sizeof(char));
	memset(lineList[c], 0 ,sizeof(lineList[c]));
    lineList[c] = pch;
	//strcpy(lineList[c], pch);
    for (i = 0; i < strlen(lineList[c]); i++) {
        if (debugMode == 1) {
        printf("lineList[c] length  : %lu\n", strlen(lineList[c]));
        printf("char %i : %c\n",i,lineList[c][i]);
        }
        if (lineList[c][i] == '(') {
            posFirstParenthesis = i;
            if (debugMode == 1) {
            printf("posFirstParenthesis: %i\n",posFirstParenthesis);
            }
            for (i2 = i; i2 < strlen(lineList[c]); i2++ ){
                if (lineList[c][i2] == ")"[0]) {
                posLastParenthesis = i2;
                if (debugMode == 1) {
                printf("posLastParenthesis: %i\n",posLastParenthesis);
                }
	            break;
                }
            }
        }
	    else if (lineList[c][i] == "\""[0]) {
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
	    for (i2 = i+ 1; i2 < strlen(lineList[c]); i2++) {
	    if (lineList[c][i2] == "\""[0]) {
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
