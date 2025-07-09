#include <stdio.h>
#include <stdlib.h>
#include <string.h>
	
int main (int argv, char **argc) {
	if (argv<2) {
		printf("wzip: file1 [file2 ...]\n");
		exit(1);
	}
	int buf_size = 1024;
	char *buf = malloc(buf_size);
	size_t buf_len = 0;
	if (buf==NULL) {
		fprintf(stderr, "malloc");
		exit(1);
	}

	for (int i=1; i<argv; i++) {

		FILE *fp = fopen(argc[i], "r");
		if (fp == NULL) {
			printf("wzip: cannot open file\n");
			exit(1);
		}
		char *line = NULL;
		size_t size;
		ssize_t read;
		while ((read = getline(&line, &size, fp)) != -1) {
			if (buf_len + read >= buf_size) {
				buf_size = (buf_len + read) *2;
				char *temp= realloc(buf, buf_size);
				if (!temp) {
					free(line);
					fclose(fp);
					free(buf);
					perror("realloc");
					exit(1);
				}
				buf = temp;
			}
			memcpy(buf+buf_len, line, read);
			buf_len += read;
		}						
		free(line);
		fclose(fp);
	}	
	if (buf_len == 0) return 0;
	int occ = 1;
	char c = buf[0];
	for (int i=1; i<buf_len; i++) {
			
		if (c != buf[i]){
			fwrite(&occ, sizeof(int), 1, stdout);
			fwrite(&c, sizeof(char), 1, stdout);
			c = buf[i];
			occ = 1;
		}
		else {
			occ++;
		}
	}
	fwrite(&occ, sizeof(int), 1, stdout);
	fwrite(&c, sizeof(char), 1, stdout);
						
	free(buf);
	return 0; 
}
