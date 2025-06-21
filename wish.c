#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argv, char **argc) {
	int rc = fork();
	if (rc == 0) {
		char *args[3];
		args[0] = "/bin/ls";
		args[1] = NULL;
		args[2] = NULL;
        execv(args[0], args);		
	}
	wait(NULL);
	printf("child finished executing the cmd\n");
    return 0;
}

