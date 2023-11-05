/*
* Shell.c
* A Simple Unix Shell
* Authors : Aloysious Kok & Gerald
* Last Update :
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "parser.h"

#define MAX_PROMPT_LENGTH 100
#define MAX_COMMAND_LENGTH 1024

char prompt[MAX_PROMPT_LENGTH] = "% "; // Default shell prompt

// Function to change the shell prompt
void change_prompt(char *new_prompt) {
    if (new_prompt && strlen(new_prompt) < MAX_PROMPT_LENGTH) {
        strcpy(prompt, new_prompt);
        strcat(prompt, " "); // Add a space for readability
    } else {
        fprintf(stderr, "Prompt is too long. Maximum length is %d characters.\n", MAX_PROMPT_LENGTH);
    }
}

// Function to display the shell prompt and get the command from the user
void get_command(char *command) {
    printf("%s", prompt);
    if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
        if (feof(stdin)) {
            // End of file (user pressed Ctrl+D)
            printf("\n");
            exit(0);
        } else {
            perror("fgets failed");
            exit(1);
        }
    }
    // Remove newline at the end of the command
    command[strcspn(command, "\n")] = '\0';
}

// Function to parse and execute the command
void execute_command(char *command) {
    // Split the command into tokens
    char *tokens[MAX_COMMAND_LENGTH / 2 + 1]; // Each token is at least two characters
    int i = 0;
    char *token = strtok(command, " ");
    while (token != NULL) {
        tokens[i++] = token;
        token = strtok(NULL, " ");
    }
    tokens[i] = NULL; // NULL-terminate the array

    // Check for the 'prompt' built-in command
    if (tokens[0] && strcmp(tokens[0], "prompt") == 0 && tokens[1]) {
        change_prompt(tokens[1]);
    } else {
        // This is where you would add additional built-in commands or external command execution
        printf("Executing: %s\n", command);
        // You would normally use fork(), exec(), and wait() to execute the command here
    }
}

int main() {
    char command[MAX_COMMAND_LENGTH];

    while (1) {
        get_command(command);
        execute_command(command);
    }

    return 0;
}
