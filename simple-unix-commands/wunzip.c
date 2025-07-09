/*
 * 
The **wunzip** tool simply does the reverse of the **wzip** tool, taking
in a compressed file and writing (to standard output again) the uncompressed
results. For example, to see the contents of **file.txt**, you would type:

```
prompt> ./wunzip file.z
```

**wunzip** should read in the compressed file (likely using **fread()**)
and print out the uncompressed output to standard output using **printf()**.

**Details**

* Correct invocation should pass one or more files via the command line to the 
  program; if no files are specified, the program should exit with return code
  1 and print "wzip: file1 [file2 ...]" (followed by a newline) or
  "wunzip: file1 [file2 ...]" (followed by a newline) for **wzip** and
  **wunzip** respectively. 
* The format of the compressed file must match the description above exactly
  (a 4-byte integer followed by a character for each run).
* Do note that if multiple files are passed to **wzip*, they are compressed
  into a single compressed output, and when unzipped, will turn into a single
  uncompressed stream of text (thus, the information that multiple files were
  originally input into **wzip** is lost). The same thing holds for
  **wunzip**. 


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argv, char **argc) {
	if (argv<2) {
		printf("wunzip: file1 [file2 ...]\n");
		exit(1);
	}
	for (int j=1;j<argv; j++) {
		
		FILE *fp = fopen(argc[j], "r");
		if (!fp) {
			perror("open");
			exit(1);
		}
		int count;
		char c;
		while (fread(&count, sizeof(int), 1, fp) == 1 && fread(&c, sizeof(char),1, fp) == 1) {
			for (int i=0;i<count; i++) printf("%c", c);
		}
		fclose(fp);
	}
	return 0;
}
	
