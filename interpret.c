#include "interpret.h"   
#include "libs/startswith.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "libs/removeCharFromString.h"
                       

int interpret(char filename[]) {
    int i;
    int i2;
    FILE *fptr;
    char line[40];
    fptr = fopen(filename, "r");
    if (fptr == NULL)
    {
        printf("Error! The file is empty\n");   
        exit(1);
    }
     while (fgets(line,150, fptr)) {
        //strcpy(line, removeCharFromString("\t", line));
        printf("%s", line);
        char line2[40];
        strcpy(line2, line);
        int c = 0;
        int posLastQuote = -1;
	int posFirstQuote;
        int posFirstParenthesis;
        int posLastParenthesis;
        int sizeLineList = 0;
        char lineList[10][10];
        char *pch = strtok(line," ");
        while (pch != NULL)
	    {
            sizeLineList++;
            printf ("pch : %s\n",pch);
	    strcpy(lineList[c], pch);
       for (i = 0; i < strlen(lineList[c]); i++) {
            printf("char : %c\n",lineList[c][i]);
            if (lineList[c][i] == "("[0]) {
                posFirstParenthesis = i;
                printf("posFirstParenthesis: %i\n",posFirstParenthesis);
                for (i2 = i; i2 < strlen(lineList[c]); i2++ )
                {
                    if (lineList[c][i2] == ")"[0]) {
                        posLastParenthesis = i2;
                        printf("posLastParenthesis: %i\n",posLastParenthesis);
		        break;

                    }
                }
            }
	    else if (lineList[c][i] == "\""[0]) {
	    if (posLastQuote == i) {
	    printf("end quote so pass it\n");
	    }
	    else {
	    posFirstQuote = i;
	    printf("posFirstQuote: %i\n", posFirstQuote);    
	    for (i2 = i+ 1; i2 < strlen(lineList[c]); i2++) {
	    if (lineList[c][i2] == "\""[0]) {
	    posLastQuote = i2;
	    printf("posLastQuote: %i\n", posLastQuote);
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
        }

    return 0;
}
