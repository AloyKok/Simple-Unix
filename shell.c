#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_COMMAND_LINE_ARGS 100
#define MAX_ARGUMENTS_PER_COMMAND 1000

void execute_command(char *command) {
    char *args[MAX_COMMAND_LINE_ARGS];
    // Tokenize the command
    char *token = strtok(command, " ");
    int i = 0;
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL; // Last element should be NULL for execvp

    pid_t pid = fork(); // Creating a new process
    if (pid == 0) {
        execvp(args[0], args);
        perror("execvp"); // returns only on error
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork");
    }
}

int main() {
    char command[MAX_ARGUMENTS_PER_COMMAND * MAX_COMMAND_LINE_ARGS];

    while (1) {
        printf("%s", "> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0; // Remove the newline character

        execute_command(command);
    }

    return 0;
}
