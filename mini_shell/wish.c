#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define PATH_MAX 100
#define SEARCH_MAX 10
#define MAX_CMDS 100
#define MAX_TOKENS 100
#define MAX_LENGTH_TOKEN 100
#define NPROCS 64

char *search_path[SEARCH_MAX];
int nums_path = 0;

void restore_std(int STD_FILENO, int saved_fd) {
    if (dup2(saved_fd, STD_FILENO) < 0)  {
        perror("dup2");
        exit(EXIT_FAILURE);
    }
}

void add_path(char *path) {
    search_path[nums_path] = malloc(strlen(path)+1);
    if (!search_path[nums_path]) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strcpy(search_path[nums_path], path);
    nums_path++;
}

int redirectTo(char *output_file,int STD_FILENO, int flags, mode_t mode) {
    int fd;
    fd = open(output_file, flags, mode);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int saved_std = dup(STD_FILENO);

    if (dup2(fd, STD_FILENO)<0) {
        perror("dup2");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    return saved_std;
}

void parseInput(char *cmd, char **tokens, char *delim) {
    if (!cmd) {
        fprintf(stderr, "Imput: bad args\n");
        exit(EXIT_FAILURE);
    }
    int i = 0;
    char *token = strtok(cmd, delim);
    while (token) {
        tokens[i++] = token;
        token = strtok(NULL, delim);
    }
    tokens[i] = NULL;
}

pid_t fork_cmd(char **tokens) {
    if (!nums_path) {
        fprintf(stderr, "Search path is empty\n");
        return -1;
    }
    int rc = fork();
    if (rc<0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (rc == 0) {
        char path[PATH_MAX];
        for (int j=0; j<nums_path; j++) {
            if ((strlen(search_path[j]) + strlen(tokens[0])+2) > PATH_MAX) {
                fprintf(stderr, "path file too big\n");
                continue;
            }
            sprintf(path, "%s/%s", search_path[j], tokens[0]);
            if (!access(path, X_OK)) { 
                execv(path, tokens);
            }
        }
        fprintf(stderr, "%s: Command not found\n", tokens[0]);
        exit(EXIT_FAILURE);
    }
    return rc; // return the pid of the child proc
}


int main(int argc, char **argv) {

	int saved_stdout = -1;
    int batch_mode = 0;
	char *init_cmd = NULL;
	size_t n;	
	char current_dir_path[PATH_MAX];
	char * cmds[MAX_CMDS];
	
    getcwd(current_dir_path, PATH_MAX);

    if (argc > 1) {
        batch_mode = 1;
        redirectTo(argv[1], STDIN_FILENO, O_RDONLY, 0);
    }

    if (!batch_mode)
        printf("%s > ", current_dir_path);

    add_path("/bin");        // Initialize path 

	while(getline(&init_cmd, &n, stdin) != -1) {
		init_cmd[strcspn(init_cmd, "\n")] = '\0'; 	// Replace '\n' with '\0'	
        
        char *temp[MAX_TOKENS];
        parseInput(init_cmd, temp, ">");
        
        if (temp[2]) {
            fprintf(stderr, "command is incorrect (>)\n");
            exit(EXIT_FAILURE);
        }

        char *cmd = temp[0]; 
        char *output_file = temp[1];

        if (output_file) {
            while (isspace(*output_file)) output_file++; // Removing leading space
            saved_stdout = redirectTo(output_file, STDOUT_FILENO, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            redirectTo(output_file, STDOUT_FILENO, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        }
        // Parallel commands
		if (strchr(cmd, '&')) {
            parseInput(cmd, cmds, "&");
		}

		else {
			cmds[0] = cmd; 
			cmds[1] = NULL;
		}


        // Execute the commands
		for (int k = 0; cmds[k]; k++) {
            cmd = cmds[k];
            
	        char *tokens[MAX_TOKENS];              
            parseInput(cmd, tokens, " ");         // Parse the input command

            
            // Command exit
            if (strcmp(tokens[0], "exit") == 0){
                for (int i = 0; i < nums_path; i++)    // Free path before exit 
                    free(search_path[i]);
                exit(0);
            }

            // Cmd cd
            else if (strcmp(tokens[0], "cd")==0) {

                if (!tokens[1]) {
                    fprintf(stderr, "Missing argument for cd\n");
                    continue;
                }
                if (chdir(tokens[1]) < 0){
                    perror("cd");
                    continue;
                }
		        getcwd(current_dir_path, PATH_MAX);

            }

            else if (strcmp(tokens[0], "path") == 0) { // path command
                // Update search path
                for (int i = 0; i < nums_path ; i++) 
                    free(search_path[i]);
                nums_path = 0;
                for (int i = 1, j = 0; tokens[i] && nums_path <= SEARCH_MAX; i++, j++) {
                    add_path(tokens[i]);
                }
            }


            else if (strcmp(tokens[0], "getSearchPath") == 0) {
                for (int i=0; i<nums_path;i++)
                    printf("%s   ", search_path[i]);
            }


            else {                      // look for the program in search path
                fork_cmd(tokens);
            }
        }


        free(init_cmd);          // Prompt refresh after each command
        init_cmd = NULL;

        while(waitpid(-1, NULL, 0) > 0); // Wait for child procs to finish
		if (saved_stdout >= 0) {
            restore_std(STDOUT_FILENO, saved_stdout);
            restore_std(STDERR_FILENO, saved_stdout);
            close(saved_stdout);
            saved_stdout = -1;
        }
        if (!batch_mode)
		    printf("%s > ", current_dir_path);

	}
}

