#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define PATH_MAX 100
#define SEARCH_MAX 10

int main(int argv, char **argc) {
	char *cmd = NULL;
	size_t n;	
	char current_dir_path[PATH_MAX];
	char *search_path[SEARCH_MAX];
 
	getcwd(current_dir_path, PATH_MAX);
	printf("%s > ", current_dir_path);
	while(getline(&cmd, &n, stdin) != -1) {

		int saved_stdout = -1;
		// Parse the input command		
		cmd[strcspn(cmd, "\n")] = '\0'; 		
		if (strchr(cmd, '>')) {
			saved_stdout = dup(STDOUT_FILENO);
			char *temp = strtok(cmd, ">");
			char *output_file = strtok(NULL, ">");
			cmd = temp;
			while (isspace(*output_file)) output_file++;

			int fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC);
			if (fd<0) {
				perror("open");
				exit(1);
			}
			if (dup2(fd, STDOUT_FILENO)<0) {
				perror("dup");
				close(fd);
				exit(1);
			}
			close(fd);

		}
		int i =0;
		char *tokens[n];
		char *token = strtok(cmd, " "); 
		while (token != NULL && i<n){
			tokens[i++] = token;
			token = strtok(NULL, " ");
		}
		tokens[i] = NULL;
		// Command exit
		if (strcmp(tokens[0], "exit")==0) exit(0);
		// Cmd cd
		else if (strcmp(tokens[0], "cd")==0) {
			char *path = malloc(strlen(current_dir_path) + strlen(tokens[1] + 2));
			if (!path) {
				printf("error malloc");
				exit(1);
			}
			sprintf(path, "%s/%s", current_dir_path, tokens[1]);

		}
		else if (strcmp(tokens[0], "path") == 0) {
			for (int i= 1, j=0; tokens[i]; i++, j++) {
				search_path[j] = tokens[i];
				printf("path %s added to search path\n", tokens[i]);
			}
		}	
		if (saved_stdout >= 0) dup2(saved_stdout, STDOUT_FILENO);
		getcwd(current_dir_path, PATH_MAX);
		printf("%s > ", current_dir_path);

	}	
	/*int rc = fork();
	if (rc == 0) {
		char *args[3];
		args[0] = "/bin/ls";
		args[1] = NULL;
		args[2] = NULL;
        execv(args[0], args);		
	}
	wait(NULL);
	printf("child finished executing the cmd\n");
    reiturn 0;
	*/
}

