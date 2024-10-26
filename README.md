# Quash - Simple Shell Implementation in C 
## Overview  
Quash (Quick Shell) is a custom shell program implemented in C, created to understand the fundamentals of shell functionality, including command execution, process handling, and inter-process communication. It supports essential shell features such as built-in commands, process management, I/O redirection, and piping, providing a straightforward way to explore the mechanics of shell operation. 

## Design Choices
### 1. Command Parsing and Tokenization
We use strtok() to split user input into tokens, identifying commands, arguments, and operators (e.g., >, <, |). This approach simplifies the parsing by breaking input based on whitespace and specific symbols, allowing Quash to support basic command syntax.

### 2. Built-In Commands
Quash includes several built-in commands (cd, pwd, echo, exit, env, setenv). Each built-in command is handled within the main loop:

cd: Changes the current directory using chdir().
pwd: Prints the current working directory using getcwd().
echo: Prints arguments and environment variables.
env: Prints or retrieves environment variable values.
setenv: Sets an environment variable.
These commands are directly processed by Quash without creating new processes, enhancing performance and simplifying error handling.

### 3. Process Management
For non-built-in commands, Quash forks a new process using fork() and executes the command with execvp(). A global variable foreground_pid tracks the PID of the active foreground process, allowing Quash to apply custom behavior, such as timeout termination.

Foreground Processes: The shell waits for the foreground process to complete using waitpid().
Background Processes: Background commands end with &. Quash identifies these commands, and the parent process immediately returns to the prompt without waiting for child completion.\

### 4. Signal Handling
To manage signals, Quash includes custom handlers for SIGINT and SIGALRM:

SIGINT: Prevents Quash from terminating when Ctrl+C is pressed, allowing only the foreground process to be affected.
SIGALRM: Implements a 10-second timeout for long-running foreground processes. If a process exceeds this duration, Quash sends SIGKILL to terminate it, enhancing control over process runtime.

### 5. I/O Redirection
Quash supports input and output redirection:

Output (>): Redirects the command's standard output to a specified file.
Input (<): Uses a file as the command's standard input source.
During parsing, Quash identifies these symbols and modifies file descriptors in the child process using dup2().

### 6. Command Piping
Piping allows the output of one command to be the input of another (command1 | command2). Quash handles this by:

Creating a pipe and forking two child processes.
Redirecting STDOUT of the first process to the pipe and STDIN of the second process to read from it.
### 7. Error Handling and Debugging
Error handling is integrated throughout, with perror() for system call errors and fprintf() for shell-specific messages. This provides useful feedback on invalid commands, syntax errors, and system failures.

## Code Documentation
### Key Sections
Signal Handlers: sigint_handler() prevents shell termination on Ctrl+C, while sigalrm_handler() ensures long-running processes are terminated after 10 seconds.

Main Loop: Reads, tokenizes, and processes commands. Built-in commands are checked first, followed by process management for external commands.  

Redirection and Piping Logic: Handles >, <, and | through additional file descriptor manipulation and piping mechanisms, ensuring each command works independently.  

## Highlights
Efficient Parsing: The strtok()-based parsing allows flexible detection of commands, arguments, and operators, making it easy to extend.  

Custom Signal Handling: Integrates robust signal management, essential for control over process termination and shell stability.  

Clear Process Control: Background and foreground process handling ensures efficient resource usage, allowing users to continue issuing commands while background processes execute.  

## Compilation and Usage

To compile Quash, run:

```bash
gcc -o quash shell.c
```
Then, start the shell with:

```bash
./quash
```
