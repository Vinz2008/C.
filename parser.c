// FOR NOW NOT USED 
// I DON'T KNOW WHY , BUT THE LINELIST RETURNED IS EMPTY

#include "parser.h"
#include <string.h>
#include <stdio.h>

int parser(char line[40],char lineList[10][10], int* sizeLineList, int posFirstQuote, int posLastQuote, int posFirstParenthesis, int posLastParenthesis,int debugMode) {
	int c = 0;
	int i;
	int i2;
	printf("line in parser : %s\n", line);
	char *pch = strtok(line," ");
        while (pch != NULL)
	    {
            *sizeLineList++;
			printf("sizelinelist incremented\n");
            if (debugMode == 1) {
            printf ("pch : %s\n",pch);
            }
	    strcpy(lineList[c], pch);
       for (i = 0; i < strlen(lineList[c]); i++) {
           if (debugMode == 1) {
            printf("char : %c\n",lineList[c][i]);
           }
            if (lineList[c][i] == '(') {
                posFirstParenthesis = i;
                if (debugMode == 1) {
                printf("posFirstParenthesis: %i\n",posFirstParenthesis);
                }
                for (i2 = i; i2 < strlen(lineList[c]); i2++ )
                {
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
	    pch = strtok(NULL, " ");

	    c++;
	    }
	return 0;
}
