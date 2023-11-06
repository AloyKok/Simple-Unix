#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "parser.h"

#define MAX_PROMPT_LEN 100

// Default shell prompt (%)
char shell_prompt[MAX_PROMPT_LEN] = "% ";

const char *shell_cmds[] = {"prompt, pwd"};

// Function Prototypes
void run_shell();
command **process_cmd_line(char *cmd_line, int new);
void builtin_prompt(command *cmd);
void builtin_pwd();
int builtin_exit();

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

int builtin_exit()
{
    printf("\nExiting Simple Unix Shell..\n");
    exit(EXIT_SUCCESS);
}