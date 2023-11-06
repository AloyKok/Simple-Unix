#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parser.h"

#define PROMPT_MAX_LEN 100

// Function prototypes
void run_shell();
void execute_command(const char* cmd);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum length for a command
#define MAX_COMMAND_LENGTH 100

// Function prototypes
void run_shell();
void execute_command(const char* cmd);

int main() {
    // Start the shell
    run_shell();
    return 0;
}

void run_shell() {
    char command[MAX_COMMAND_LENGTH];
    char prompt[MAX_COMMAND_LENGTH] = "% ";

    while (1) {
        printf("%s", prompt); // Display the prompt
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            printf("Error reading command. Exiting shell.\n");
            break;
        }

        // Remove newline character from command if present
        command[strcspn(command, "\n")] = 0;

        // Check if the command is to change the prompt
        if (strncmp(command, "prompt ", 7) == 0) {
            // Change the shell prompt to the second token
            strcpy(prompt, command + 7);
            size_t len = strlen(prompt);
            if (len == 0 || prompt[len - 1] != ' ') {
                // Ensure the prompt ends with a space
                strcat(prompt, " ");
            }
        } else {
            // Execute the command
            execute_command(command);
        }
    }
}

void execute_command(const char* cmd) {
    // Here you would parse the command and execute it.
    // For now, we'll just print it out.
    printf("Executing command: %s\n", cmd);
}


