#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "color.h"


#define NB_FILES 12

int main(){
	char exe[] = "../cpoint";
	char flags[] = "-std ../std -c";
	char* cmd = NULL;
	FILE* cmdf;
	for (int i = 0; i < NB_FILES; i++){
		int size = strlen(exe) + 1 + strlen(flags) + 1 + 7 + 1 + 7 + 1 + 1;
		cmd = malloc(size * sizeof(char));
		snprintf(cmd, size, "%s %s ../test%d.cpoint", exe, flags, i+1);
		printf("cmd : %s\n", cmd);
		cmdf = popen(cmd, "r");
		char* out = malloc(1000 * sizeof(char));
		//fscanf(cmdf, "%s", out);
		fread(out, 1, 1000, cmdf);
		//printf("out : %s\n", out);
		int stat_temp = pclose(cmdf);
		int status = WEXITSTATUS(stat_temp);
		printf("status returned : %d\n", status);
		if (status == 1){
			printf(RED "Test %d failed\n" CRESET, i+1);
			exit(1);
		} else {
			printf(GRN "Test %d passed\n" CRESET, i+1);
		}
		free(cmd);
		free(out);
	}
	printf(BLU"Every tests passed\n"CRESET);
	return 0;
}
