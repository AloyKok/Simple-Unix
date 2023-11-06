#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h" 

#define MAX_PROMPT_LEN 100
char shell_prompt[MAX_PROMPT_LEN] = "% ";

// Function Prototypes
void run_shell();
command** process_cmd_line(char *cmd_line, int new);

int main() {
    run_shell(); // Start the shell
    return 0;
}

void run_shell() {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    command **cmd_line;

    printf("%s", shell_prompt); // Display the default prompt

    while ((nread = getline(&line, &len, stdin)) != -1) {
        // Remove newline character from line if present
        if(line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        // Process the command line
        cmd_line = process_cmd_line(line, 1);

        int i;
        // Execute the command line
        // This is where you would add the execution logic using the parsed command line
        // For now, it will just print the parsed structure for demonstration purposes
        if (cmd_line) {
            int i = 0;
            while (cmd_line[i] != NULL) {
                dump_structure(cmd_line[i], i); // This is a function from your parser.c that prints the command structure
                i++;
            }
        }

        // Free the memory allocated by getline() and the command structures
        if (line) {
            free(line);
            line = NULL;
        }
        clean_up(cmd_line, i); // Assuming clean_up() frees the entire command line structure array

        printf("%s", shell_prompt); // Display the default prompt
    }

    free(line); // Free the memory allocated by getline()
}