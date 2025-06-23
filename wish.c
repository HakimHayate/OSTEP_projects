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
	 
	char *tokens[n];
	int nb_path = 0;
	getcwd(current_dir_path, PATH_MAX);
	printf("%s > ", current_dir_path);
	while(getline(&cmd, &n, stdin) != -1) {

		int saved_stdout = -1;
		// Redirect stdout to file if '>' is present		
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
		// Parse input command
		int i =0;
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
			char *path = malloc(strlen(current_dir_path) + strlen(tokens[1]) + 2);
			if (!path) {
				perror("error malloc");
				exit(1);
			}
			sprintf(path, "%s/%s", current_dir_path, tokens[1]);
			if (chdir(path)<0){
					perror("cd");
					exit(1);
				}

		}
		else if (strcmp(tokens[0], "path") == 0) {
			nb_path = 0;
			for (int i= 1, j=0; tokens[i]; i++, j++) {
				nb_path++;
				search_path[j] = malloc(strlen(tokens[i]+1));
				if (!search_path[j]) {
					perror("malloc");
					exit(1);
				}
				stpcpy(search_path[j], tokens[i]);
				printf("path %s added to search path\n", tokens[i]);
			}
		}
		else if (strcmp(tokens[0], "getSearchPath") == 0) {
			for (int i=0; i<nb_path;i++)
				printf("%s \n", search_path[i]);
			
		}
		// look for program in search path
		else {
			int rc = fork();
			if (rc<0) {
				perror("fork");
				exit(1);
			}
			if (rc == 0) {
				for (int j=0; j<nb_path; j++) {
					char *path = malloc(strlen(search_path[j]) + strlen(tokens[0]+2));
					sprintf(path, "%s/%s", search_path[j], tokens[0]);
					execv(path, tokens);
					}
				printf("Command not found\n");
				exit(1);
				}
			else {
				wait(NULL);
			}	
		}	
		if (saved_stdout >= 0) dup2(saved_stdout, STDOUT_FILENO);
		getcwd(current_dir_path, PATH_MAX);
		printf("%s > ", current_dir_path);

	}
}

