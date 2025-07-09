#include <stdio.h>
#include <stdlib.h>

int main(int argv, char ** argc) {
	if (argv < 2) return 0;
	for (int i=1; i<argv; i++) {
		
		FILE *fp = fopen(argc[i], "r");
		if (fp == NULL){
			printf("wcat: cannot open file\n");
			exit(1);
			}
		char buffer[100];

	        while(fgets(buffer, 100, fp)) {
			printf("%s", buffer);
		}
		fclose(fp);
	}
	return 0;
}	

