#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

// This function will print out the command structure for verification
void print_command_structure(command *cmd_struct)
{
    printf("Command Name: %s\n", cmd_struct->com_name);
    printf("Arguments:\n");
    for (int i = 0; cmd_struct->argv[i] != NULL; i++)
    {
        printf("  argv[%d]: %s\n", i, cmd_struct->argv[i]);
    }
    printf("Background: %d\n", cmd_struct->background);
    printf("Sequential: %d\n", cmd_struct->sequential);
    printf("Redirect In: %s\n", cmd_struct->redirect_in);
    printf("Redirect Out: %s\n", cmd_struct->redirect_out);
    printf("Pipe To: %d\n", cmd_struct->pipe_to);
}

// Modified test_process_cmd function
void test_process_cmd() {
    printf("============== PROCESS CMD TEST =============== \n");

    const char *test_commands[] = {
        "ls -l > output.txt",
        "cat < input.txt",
        // ... (rest of your command strings)
        NULL // Marks the end of the test commands
    };

    // Test each command
    for (int i = 0; test_commands[i] != NULL; i++) {
        char *command_copy = strdup(test_commands[i]);
        if (command_copy == NULL) {
            perror("strdup failed");
            continue;
        }

        command *cmd_struct = calloc(1, sizeof(command));
        printf("Testing command: %s\n", command_copy);
        process_cmd(command_copy, cmd_struct); // Process the command

        print_command_structure(cmd_struct); // Function to print the command structure
        clean_up(&cmd_struct, 1); // Clean up the command structure

        free(command_copy); // Free the allocated command copy
        printf("-------------------------------\n");
    }
}

// Modified test_process_cmd_line function
void test_process_cmd_line() {
    printf("============== PROCESS CMD LINE TEST =============== \n");
    const char *test_command_lines[] = {
        "ls -l > output.txt & cat < input.txt",
        "grep 'main' < input.c > output.txt ; gcc -o program program.c",
        "find . -name '*.c' | xargs grep 'stdio'",
        "echo hello && ls -l", // This should fail the check due to consecutive separators
        "   ls -l",            // This should fail the check due to leading whitespace
        "",                    // This should fail the check due to being empty
        NULL                   // Marks the end of the test command lines
    };

    for (int i = 0; test_command_lines[i] != NULL; i++) {
        char *command_line_copy = strdup(test_command_lines[i]);
        if (command_line_copy == NULL) {
            perror("strdup failed");
            continue;
        }

        printf("Testing command line: %s\n", command_line_copy);
        int check_result = check_cmd_input(command_line_copy);
        if (check_result != 0) {
            printf("Command input check failed with code: %d\n", check_result);
        } else {
            command **cmd_array = process_cmd_line(command_line_copy, 1); // Process the command line

            int j;
            // Print and clean up the command structures
            for (int j = 0; cmd_array && cmd_array[j] != NULL; j++) {
                print_command_structure(cmd_array[j]);
            }
            clean_up(cmd_array, j); // Clean up the command structures
            free(cmd_array); // Free the array itself
        }

        free(command_line_copy); // Free the allocated command line copy
        printf("-------------------------------\n");
    }
}

int main()
{
    test_process_cmd();      // Test individual commands
    test_process_cmd_line(); // Test entire command lines
    return 0;
}
