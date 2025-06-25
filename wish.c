#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define PATH_MAX 100
#define SEARCH_MAX 10
#define MAX_CMDS 100
#define MAX_TOKENS 100
int main(int argv, char **argc) {
	char *init_cmd = NULL;
	size_t n;	
	char current_dir_path[PATH_MAX];
	char *search_path[SEARCH_MAX];
	char * cmds[MAX_CMDS];
	char *tokens[MAX_TOKENS];
	int nb_path = 0;
	getcwd(current_dir_path, PATH_MAX);
	printf("%s > ", current_dir_path);
	while(getline(&init_cmd, &n, stdin) != -1) {

		int saved_stdout = -1;
		// Redirect stdout to file if '>' is present		
		init_cmd[strcspn(init_cmd, "\n")] = '\0'; 		
        char *cmd = strtok(init_cmd, ">");
        char *output_file = strtok(NULL, ">");
        if (output_file) {
            while (isspace(*output_file)) output_file++;

            int fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd<0) {
                perror("open");
                exit(1);
            }
            
            saved_stdout = dup(STDOUT_FILENO);
            if (dup2(fd, STDOUT_FILENO)<0) {
                perror("dup");
                close(fd);
                exit(1);
            }
            close(fd);
        }

        // Parallel commands
		if (strchr(cmd, '&')) {
			int j = 0;
		   	char *temp = strtok(cmd, "&");
			while (temp && j < MAX_CMDS - 1) {
				cmds[j++] = temp;
				temp = strtok(NULL, "&");
			}
			cmds[j] = NULL;
		}
		else {
			cmds[0] = cmd; 
			cmds[1] = NULL;
		}
        // Execute the commands
        int num_procs = 0;
		for (int k = 0; cmds[k]; k++) {
            num_procs++;    
            cmd = cmds[k];
            printf("cmd = %s\n", cmd);
            // Parse input command
            int i = 0;
            char *token = strtok(cmd, " "); 
            while (token != NULL && i < MAX_TOKENS - 1){ // Last token has to be NULL
                tokens[i++] = token;
                token = strtok(NULL, " ");
            }
            tokens[i] = NULL;
            // Command exit
            if (strcmp(tokens[0], "exit")==0){
                for (int i = 0; i < nb_path ; i++) 
                    free(search_path[i]);
                exit(0);
            }

            // Cmd cd
            else if (strcmp(tokens[0], "cd")==0) {

                if (!tokens[1]) {
                    printf("Missing argument for cd\n");
                    continue;
                }
                char *path = malloc(strlen(current_dir_path) + strlen(tokens[1]) + 2);
                if (!path) {
                    perror("error malloc");
                    exit(1);
                }
                sprintf(path, "%s/%s", current_dir_path, tokens[1]);
                if (chdir(path) < 0){
                    perror("cd");
                    free(path);
                    continue;
                }
                free(path);
		        getcwd(current_dir_path, PATH_MAX);

            }
            else if (strcmp(tokens[0], "path") == 0) {
                // Update search path
                for (int i = 0; i < nb_path ; i++) 
                    free(search_path[i]);
                nb_path = 0;
                for (int i = 1, j = 0; tokens[i] && nb_path<=SEARCH_MAX; i++, j++) {
                    nb_path++;
                    search_path[j] = malloc(strlen(tokens[i])+1);
                    if (!search_path[j]) {
                        perror("malloc");
                        exit(1);
                    }
                    strcpy(search_path[j], tokens[i]);
                    printf("path %s added to search path\n", tokens[i]);
                }
            }
            else if (strcmp(tokens[0], "getSearchPath") == 0) {
                for (int i=0; i<nb_path;i++)
                    printf("%s \n", search_path[i]);
            }
            // look for program in search path
            else {
                if (!nb_path) {
                    fprintf(stderr, "Search path is empty\n");
                    continue;
                }
                int rc = fork();
                if (rc<0) {
                    perror("fork");
                    exit(1);
                }
                if (rc == 0) {
                    char *path;
                    for (int j=0; j<nb_path; j++) {
                        path = malloc(strlen(search_path[j]) + strlen(tokens[0])+2);
                        sprintf(path, "%s/%s", search_path[j], tokens[0]);
                        execv(path, tokens);
                        free(path);
                    }
                    printf("Command not found\n");
                    exit(1);
                }
            }
        }
        // Prompt refresh after each command 
        free(init_cmd);
        init_cmd = NULL;
        // Wait for child procs to finish
        for (int i=0; i<num_procs;i++) wait(NULL);
		if (saved_stdout >= 0) {
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
		printf("%s > ", current_dir_path);

	}
}

