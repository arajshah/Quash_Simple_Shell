#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_COMMAND_LINE_LEN 1024
#define MAX_COMMAND_LINE_ARGS 128

char prompt[] = "> ";
char delimiters[] = " \t\r\n";
extern char **environ;
pid_t foreground_pid = -1;

// Signal handler function for SIGINT
void sigint_handler(int sig) {
    // Ignore the signal (do nothing)
    // Optionally, you can print a message or re-display the prompt
    printf("\n");
    fflush(stdout);
}

// Signal handler function for SIGALRM
void sigalrm_handler(int sig) {
    if (foreground_pid > 0) {
        printf("\nProcess %d exceeded time limit. Terminating it.\n", foreground_pid);
        kill(foreground_pid, SIGKILL); // Terminate the child process 
    }
}

int main() {
    // Register the SIGINT handler
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal error");
        exit(EXIT_FAILURE);
    }

    // Register the SIGALRM handler
    if (signal(SIGALRM, sigalrm_handler) == SIG_ERR) {
        perror("signal error");
        exit(EXIT_FAILURE);
    }

    // Stores the string typed into the command line.
    char command_line[MAX_COMMAND_LINE_LEN];
  
    // Stores the tokenized command line input.
    char *arguments[MAX_COMMAND_LINE_ARGS];
    	
    while (true) {
      
        // Print the shell prompt
        char cwd[MAX_COMMAND_LINE_LEN];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s> ", cwd);
        } else {
            perror("getcwd() error");
            continue;
        }
        fflush(stdout);

        // Read input from stdin and store it in command_line.
        if ((fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) && ferror(stdin)) {
            fprintf(stderr, "fgets error");
            exit(0);
        }
 
        // Remove the trailing newline character
        command_line[strcspn(command_line, "\n")] = '\0';
      
        // If the user input was EOF (ctrl+d), exit the shell.
        if (feof(stdin)) {
            printf("\n");
            fflush(stdout);
            fflush(stderr);
            exit(0);
        }

        // Skip empty commands
        if (strlen(command_line) == 0) {
            continue;
        }
        
        // 1. Tokenize the command line input (split it on whitespace)
        int argc = 0; // Argument Count
        char *token = strtok(command_line, delimiters);
        while (token != NULL && argc < MAX_COMMAND_LINE_ARGS -1) {
            // Check for environment variables starting with $
            if (token[0] == '$') {
                char *env_var = getenv(token + 1); // Skip the $ character
                if (env_var != NULL) {
                    arguments[argc] = env_var;
                } else {
                    arguments[argc] = ""; // Empty string if variable is not found
                }
            } else {
                arguments[argc] = token;
            }
            argc++;
            token = strtok(NULL, delimiters);
        }
        arguments[argc] = NULL; // Null-terminate the arguments array
      
        // 2. Implement Built-In Commands
        // Check if command is not empty 
        if (arguments[0] == NULL) {
            // Empty command
            continue;
        }

        // Check for built-in Commands
        if (strcmp(arguments[0], "cd") == 0) {
            // Handle 'cd' command
            if (arguments[1] != NULL) {
                if (chdir(arguments[1]) != 0) {
                    perror("chdir error");
                }
            } else {
                fprintf(stderr, "cd: expected argument\n");
            }
            continue;
        } else if (strcmp(arguments[0], "pwd") == 0) {
            // Handle 'pwd' command
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("getcwd() error");
            }
            continue;
        } else if (strcmp(arguments[0], "echo") == 0) {
            // Handle "echo" command
            for (int i = 1; i < argc; i++) {
                printf("%s ", arguments[i]);
            }
            printf("\n");
            continue;
        } else if (strcmp(arguments[0], "exit") == 0) {
            // Handle exit command
            exit(0);
        } else if (strcmp(arguments[0], "env") == 0) {
            // Handle 'env' command
            if (arguments[1] != NULL) {
                char *env_value = getenv(arguments[1]);
                if (env_value != NULL) {
                    printf("%s=%s\n", arguments[1], env_value);
                } else {
                    printf("%s not found\n", arguments[1]);
                }
            } else {
                for (char **env = environ; *env != NULL; env++) {
                    printf("%s\n", *env);
                }
            }
            continue;
        } else if (strcmp(arguments[0], "setenv") == 0) {
            // Handle "setenv" command
            if (arguments[1] != NULL && arguments[2] != NULL) {
                if (setenv(arguments[1], arguments[2], 1) != 0) {
                    perror("setenv error");
                }
            } else {
                fprintf(stderr, "setenv: expected variable and value\n");
            }
            continue;
        }

        // Handle external commands

        // Check for background process
        int background = 0;
        if (argc > 0 && strcmp(arguments[argc - 1], "&") == 0) {
            background = 1;
            arguments[argc - 1] = NULL; // Remove "&" from arguments
            argc--;
        }

        // Implement I/O redirection and piping
        // Initialize variables
        int redirect_output = 0;
        char *output_file = NULL;
        int redirect_input = 0;
        char *input_file = NULL;
        int piping = 0;
        char *cmd1[MAX_COMMAND_LINE_ARGS];
        char *cmd2[MAX_COMMAND_LINE_ARGS];

        // Check for pipe '|'
        char *pipe_pos = NULL;
        for (int i = 0; i < argc; i++) {
            if (strcmp(arguments[i], "|") == 0) {
                piping = 1;
                arguments[i] = NULL; // Split the arguments array
                // Set up cmd1
                for (int j = 0; j < i; j++) {
                    cmd1[j] = arguments[j];
                }
                cmd1[i] = NULL;
                // Set up cmd2
                int k = 0;
                for (int j = i + 1; j < argc; j++) {
                    cmd2[k++] = arguments[j];
                }
                cmd2[k] = NULL;
                break;
            }
        }

        if (piping) {
            // Implement piping
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe error");
                continue;
            }

            pid_t pid1 = fork();
            if (pid1 < 0) {
                perror("fork error");
                continue;
            }

            if (pid1 == 0) {
                // First child process (cmd1)
                // Restore default signals
                signal(SIGINT, SIG_DFL);
                signal(SIGALRM, SIG_DFL);

                // Redirect stdout to pipe
                close(pipefd[0]); // Close unused read end
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]); // Close original write end

                if (execvp(cmd1[0], cmd1) == -1) {
                    perror("execvp error");
                    exit(EXIT_FAILURE);
                }
            }

            pid_t pid2 = fork();
            if (pid2 < 0) {
                perror("fork error");
                continue;
            }

            if (pid2 == 0) {
                // Second child process (cmd2)
                // Restore default signals
                signal(SIGINT, SIG_DFL);
                signal(SIGALRM, SIG_DFL);

                // Redirect stdin to pipe
                close(pipefd[1]); // Close unused write end
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]); // Close original read end

                if (execvp(cmd2[0], cmd2) == -1) {
                    perror("execvp error");
                    exit(EXIT_FAILURE);
                }
            }

            // Parent process
            close(pipefd[0]);
            close(pipefd[1]);

            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);

            // No need to set alarm or foreground_pid since the shell is not blocked waiting for a single process
            continue;
        }

        // Check for output redirection '>'
        for (int i = 0; i < argc; i++) {
            if (strcmp(arguments[i], ">") == 0) {
                redirect_output = 1;
                if (i + 1 < argc) {
                    output_file = arguments[i + 1];
                    arguments[i] = NULL; // Remove '>' and filename from arguments
                    argc = i;
                } else {
                    fprintf(stderr, "Syntax error: expected filename after '>'\n");
                    redirect_output = 0;
                }
                break;
            }
        }

        // Check for input redirection '<'
        for (int i = 0; i < argc; i++) {
            if (strcmp(arguments[i], "<") == 0) {
                redirect_input = 1;
                if (i + 1 < argc) {
                    input_file = arguments[i + 1];
                    arguments[i] = NULL; // Remove '<' and filename from arguments
                    argc = i;
                } else {
                    fprintf(stderr, "Syntax error: expected filename after '<'\n");
                    redirect_input = 0;
                }
                break;
            }
        }

        // 3. Create a child process which will execute the command line input
        pid_t pid = fork();
        if (pid < 0) {
            // Fork failed
            perror("fork error");
        } else if (pid == 0) {
            // Restore default SIGINT and SIGALRM behavior
            signal(SIGINT, SIG_DFL);
            signal(SIGALRM, SIG_DFL);

            // Handle output redirection
            if (redirect_output) {
                int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("open error");
                    exit(EXIT_FAILURE);
                }
                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("dup2 error");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
                close(fd);
            }

            // Handle input redirection
            if (redirect_input) {
                int fd = open(input_file, O_RDONLY);
                if (fd < 0) {
                    perror("open error");
                    exit(EXIT_FAILURE);
                }
                if (dup2(fd, STDIN_FILENO) < 0) {
                    perror("dup2 error");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
                close(fd);
            }

            // Child process
            if (execvp(arguments[0], arguments) == -1) {
                perror("execvp error");
            }
            exit(EXIT_FAILURE); // Exit child process
        } else {
            // 4. The parent process should wait for the child to complete unless it's a background process
            // Parent process
            if (!background) {
                foreground_pid = pid; // Set the global foreground_pid
                alarm(10); // Set the alarm for 10 seconds

                int status;
                waitpid(pid, &status, 0); // Wait for child to finish

                alarm(0); // Cancel the alarm
                foreground_pid = -1;  // Reset the foreground_pid
            } else {
                printf("Process running in background with PID %d\n", pid);
            }
        }

        // Hints (put these into Google):
        // man fork
        // man execvp
        // man wait
        // man strtok
        // man environ
        // man signals
        
        // Extra Credit
        // man dup2
        // man open
        // man pipes
    }
    // This should never be reached.
    return -1;
}
