#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "parser.h"

#define MAX_PROMPT_LEN 100

// Default shell prompt (%)
char shell_prompt[MAX_PROMPT_LEN] = "% ";

const char *shell_cmds[] = {"prompt", "cd", "help" "pwd", "exit"};

// Function Prototypes
void run_shell();
command **process_cmd_line(char *cmd_line, int new);
void builtin_prompt(command *cmd);
void builtin_cd(command *cmd);
void builtin_pwd();
int builtin_exit();
int exec_pipe(command **cmd_stack, int current) ;

int main()
{
    run_shell(); // Start the shell
    return 0;
}

void run_shell()
{
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    command **cmd_line;

    printf("%s", shell_prompt); // Display the default prompt

    while ((nread = getline(&line, &len, stdin)) != -1)
    {
        // Remove newline character from line if present
        if (line[nread - 1] == '\n')
        {
            line[nread - 1] = '\0';
        }

        // Process the command line
        cmd_line = process_cmd_line(line, 1);

        int i;

        // Execute the command line
        if (cmd_line)
        {
            int i = 0;
            while (cmd_line[i] != NULL)
            {
                dump_structure(cmd_line[i], i); // This is a function from your parser.c that prints the command structure
                i++;
            }
        }

        // Execute the command line
        if (cmd_line)
        {
            for (int i = 0; cmd_line[i] != NULL; i++)
            {
                // Check if the command is 'prompt'
                if (strcmp(cmd_line[i]->com_name, "pwd") == 0)
                {
                    builtin_pwd(); // Call the function to print the working directory
                }
                else if (strcmp(cmd_line[i]->com_name, "prompt") == 0)
                {
                    builtin_prompt(cmd_line[i]); // Change the prompt
                }
                else if (strcmp(cmd_line[i]->com_name, "cd") == 0)
                {
                    builtin_cd(cmd_line[i]); // Change the directory
                }
                else if (strcmp(cmd_line[i]->com_name, "exit") == 0)
                {
                    builtin_exit();
                }
                // If this is not the last command, you might want to skip execution
                // as the prompt command doesn't need to be 'run' like others
            }
        }

        // Free the memory allocated by getline() and the command structures
        if (line)
        {
            free(line);
            line = NULL;
        }
        clean_up(cmd_line, i); // Assuming clean_up() frees the entire command line structure array

        printf("%s", shell_prompt);
    }

    free(line); // Free the memory allocated by getline()
}

// This function changes the shell prompt to the second token of the command.
void builtin_prompt(command *cmd)
{
    if (cmd->argv[1] != NULL)
    { // Check if there is a second argument
        strncpy(shell_prompt, cmd->argv[1], MAX_PROMPT_LEN - 2);
        size_t len = strlen(shell_prompt);
        shell_prompt[len] = ' ';      // Add a space after the prompt
        shell_prompt[len + 1] = '\0'; // Null-terminate the string
    }
    else
    {
        printf("prompt: expects an argument\n");
    }
}

// Function to implement the 'pwd' command
void builtin_pwd()
{
    char cwd[PATH_MAX]; // PATH_MAX is defined in limits.h and is the maximum length for a path
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s\n", cwd); // Print the current working directory
    }
    else
    {
        perror("pwd"); // Print the error message if getcwd fails
    }
}

// Function to implement the 'cd' command
void builtin_cd(command *cmd) {
    const char *path = cmd->argv[1]; // Get the second argument as the path
    char cwd[PATH_MAX]; // Buffer to store the current working directory

    if (path == NULL) {
        path = getenv("HOME"); // If no path is provided, use the HOME environment variable
    }

    // Try to change directory
    if (chdir(path) != 0) {
        perror("cd"); // Print an error message if chdir fails
    } 
    else {
        // If chdir is successful, get the new current working directory
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd); // Print the current working directory
        }
    }
}

int builtin_exit()
{
    printf("\nExiting Simple Unix Shell..\n");
    exit(EXIT_SUCCESS);
}

int exec_pipe(command **cmd_stack, int current) {
    int idx = current;
    int p_count = 0;
    int pipefd[2 * 1000]; // Support up to 1000 commands, hence 2 fds per command (input/output)
    pid_t pid;
    int stdin_backup = dup(0); // Backup for stdin
    int stdout_backup = dup(1); // Backup for stdout

    if (stdin_backup == -1 || stdout_backup == -1) {
        perror("dup");
        return -1; // Handle dup error
    }

    // Create all needed pipes first
    while (cmd_stack[idx] != NULL && cmd_stack[idx]->pipe_to > 0) {
        if (pipe(pipefd + p_count * 2) == -1) {
            perror("pipe");
            return -1; // Handle pipe error
        }
        p_count++;
        idx++;
    }

    idx = current;

    // Execute each command in the pipeline
    for (int i = 0; i <= p_count; i++) {
        int inputfile = -1;
        int outputfile = -1;

        // Handle input redirection for the first command
        if (i == 0 && cmd_stack[idx]->redirect_in != NULL) {
            inputfile = open(cmd_stack[idx]->redirect_in, O_RDONLY);
            if (inputfile == -1) {
                perror("open");
                return -1; // Handle open error
            }
        }

        // Handle output redirection for the last command
        if (i == p_count && cmd_stack[idx]->redirect_out != NULL) {
            outputfile = open(cmd_stack[idx]->redirect_out, O_WRONLY | O_CREAT | O_TRUNC, 0755);
            if (outputfile == -1) {
                perror("open");
                return -1; // Handle open error
            }
        }

        pid = fork();
        if (pid == 0) { // Child process
            // If not the first command, get input from the previous pipe
            if (i > 0) {
                if (dup2(pipefd[(i - 1) * 2], STDIN_FILENO) == -1) {
                    perror("dup2"); // Handle dup2 error
                    exit(EXIT_FAILURE);
                }
            } else if (inputfile != -1) {
                if (dup2(inputfile, STDIN_FILENO) == -1) {
                    perror("dup2"); // Handle dup2 error
                    exit(EXIT_FAILURE);
                }
                close(inputfile);
            }

            // If not the last command, output to the next pipe
            if (i < p_count) {
                if (dup2(pipefd[i * 2 + 1], STDOUT_FILENO) == -1) {
                    perror("dup2"); // Handle dup2 error
                    exit(EXIT_FAILURE);
                }
            } else if (outputfile != -1) {
                if (dup2(outputfile, STDOUT_FILENO) == -1) {
                    perror("dup2"); // Handle dup2 error
                    exit(EXIT_FAILURE);
                }
                close(outputfile);
            }

            // Close all pipes in the child process
            for (int j = 0; j < 2 * p_count; j++) {
                close(pipefd[j]);
            }

            // Execute the command
            execvp(cmd_stack[idx]->argv[0], cmd_stack[idx]->argv);
            perror("execvp"); // If execvp returns, it's an error
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork"); // Handle fork error
            return -1;
        }

        idx++;
    }

    // Close all pipes in the parent process
    for (int i = 0; i < 2 * p_count; i++) {
        close(pipefd[i]);
    }

    // Wait for all processes in the pipeline to finish
    for (int i = 0; i <= p_count; i++) {
        wait(NULL);
    }

    // Restore original stdin and stdout
    dup2(stdin_backup, 0);
    dup2(stdout_backup, 1);
    close(stdin_backup);
    close(stdout_backup);

    return 0;
}