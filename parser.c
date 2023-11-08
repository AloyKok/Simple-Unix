/*
 * Parser.c
 * A simple Command Line Parser.
 * Author : Aloysious Kok & Gerald
 * Last Modification :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"

// #define DEBUG

char *trim_whitespace(char *str)
{
    if (!str)
        return NULL;

    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0) // If all spaces
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

// Helper function to duplicate a string with error checking
char *safe_strdup(const char *str)
{
    if (!str)
        return NULL;

    char *copy = strdup(str);
    if (!copy)
    {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    return copy;
}

// Helper function to reallocate an array with error checking
void *safe_realloc(void *ptr, size_t size)
{
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr)
    {
        perror("Failed to reallocate memory");
        free(ptr); // Free the original block to avoid memory leak
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

// Function to free a command structure
void free_command(command *cmd)
{
    if (cmd)
    {
        free(cmd->com_name); // Free the command name
        cmd->com_name = NULL;

        if (cmd->argv)
        { // Free each argument in the argv array
            for (int i = 0; cmd->argv[i] != NULL; i++)
            {
                free(cmd->argv[i]);
                cmd->argv[i] = NULL; // Set freed pointer to NULL to avoid double-free
            }
            free(cmd->argv);  // Free the array itself
            cmd->argv = NULL; // Set freed pointer to NULL
        }
        free(cmd->redirect_in);  // Free the input redirection string
        cmd->redirect_in = NULL; // Set freed pointer to NULL

        free(cmd->redirect_out);  // Free the output redirection string
        cmd->redirect_out = NULL; // Set freed pointer to NULL

        if (cmd->globbuf.gl_pathv)
        {
            globfree(&cmd->globbuf); // Free the memory allocated by glob
        }

        free(cmd); // Finally, free the command structure itself
    }
}

// Function to process a simple command and fill the command struct.
void process_simple_cmd(char *cmd, command *result)
{
    if (!cmd || !result)
    {
        perror("Invalid input to process_simple_cmd");
        exit(EXIT_FAILURE);
    }

    // Trim the command string to remove leading and trailing whitespace
    cmd = trim_whitespace(cmd);

    // Initialize the argv in the result structure
    result->argv = safe_realloc(result->argv, sizeof(char *));
    result->argv[0] = NULL;

    // Split the command string into tokens based on whitespace
    char *token = strtok(cmd, " \t\n");
    if (!token)
    {
        // If there are no spaces, then the entire cmd is the command name
        result->com_name = safe_strdup(cmd);
        result->argv[0] = safe_strdup(cmd);
    }
    else
    {
        // The first token is the command name
        result->com_name = safe_strdup(token);
        int argc = 0;
        do
        {
            // Check for wildcard characters in the token
            if (strpbrk(token, "*?") != NULL)
            {
                // Wildcard detected, prepare for expansion
                if (glob(token, 0, NULL, &result->globbuf) == 0)
                {
                    // Add each expanded filename to the argv array
                    for (size_t i = 0; i < result->globbuf.gl_pathc; ++i)
                    {
                        result->argv = safe_realloc(result->argv, (argc + 2) * sizeof(char *));
                        result->argv[argc] = safe_strdup(result->globbuf.gl_pathv[i]);
                        argc++;
                    }
                }
                else
                {
                    // If glob() failed, treat the token as a regular argument
                    result->argv = safe_realloc(result->argv, (argc + 2) * sizeof(char *));
                    result->argv[argc] = safe_strdup(token);
                    argc++;
                }
            }
            else
            {
                // No wildcard, proceed as normal
                result->argv = safe_realloc(result->argv, (argc + 2) * sizeof(char *));
                result->argv[argc] = safe_strdup(token);
                argc++;
            }
        } while ((token = strtok(NULL, " \t\n")) != NULL);

        // NULL-terminate the argv array
        result->argv[argc] = NULL;
    }
}

void process_cmd(char *cmd, command *result)
{
    char *simple_cmd = NULL;
    char *redir_in_ptr, *redir_out_ptr;
    char *cmd_copy = safe_strdup(cmd); // Duplicate the command string for safe manipulation

    // Initialize the result structure to avoid uninitialized memory access
    result->com_name = NULL;
    result->argv = NULL;
    result->redirect_in = NULL;
    result->redirect_out = NULL;
    result->background = 0;
    result->sequential = 0;
    result->pipe_to = 0;

    // Look for input and output redirection characters
    redir_in_ptr = strchr(cmd_copy, '<');
    redir_out_ptr = strchr(cmd_copy, '>');

    // If input redirection is present
    if (redir_in_ptr != NULL)
    {
        *redir_in_ptr = '\0'; // Split the command at the '<' character
        simple_cmd = cmd_copy;
        result->redirect_in = safe_strdup(redir_in_ptr + 1);
        // If output redirection is also present
        if ((redir_out_ptr != NULL) && (redir_out_ptr > redir_in_ptr))
        {
            *redir_out_ptr = '\0';
            result->redirect_out = safe_strdup(redir_out_ptr + 1);
        }
    }
    // If only output redirection is present
    else if (redir_out_ptr != NULL)
    {
        *redir_out_ptr = '\0';
        simple_cmd = cmd_copy;
        result->redirect_out = safe_strdup(redir_out_ptr + 1);
    }
    // If no redirections are present
    else
    {
        simple_cmd = cmd_copy;
    }

    // Process the simple command if present
    if (simple_cmd)
    {
        process_simple_cmd(simple_cmd, result);
    }

    free(cmd_copy); // Clean up the duplicated command string
}

void clean_up(command **cmd_stack, int cmd_count)
{
    if (!cmd_stack)
    {
        return;
    }

    for (int i = 0; i < cmd_count; i++)
    {
        command *cmd = cmd_stack[i];
        if (cmd)
        {
            free(cmd->com_name);
            cmd->com_name = NULL;

            if (cmd->argv)
            {
                for (int j = 0; cmd->argv[j] != NULL; j++)
                {
                    free(cmd->argv[j]);
                    cmd->argv[j] = NULL;
                }
                free(cmd->argv);
                cmd->argv = NULL;
            }

            free(cmd->redirect_in);
            cmd->redirect_in = NULL;

            free(cmd->redirect_out);
            cmd->redirect_out = NULL;

            free(cmd);
            cmd_stack[i] = NULL; // This is crucial to prevent double free errors
        }
    }
}

command **process_cmd_line(char *cmd, int new)
{
    char *rc, *mc;
    char *rc_copy = NULL;
    static command **cmd_line;
    static int lc;
    char sep;

    if (new == 1)
    {
        lc = 0;
        cmd_line = NULL;
    }

    cmd = trim_whitespace(cmd);

    /*
     * Check for the existance of delimitors.
     * If no delimitors exist, we only have one command on the command line.
     * Otherwise process accordingly.
     */
    sep = lead_separator(cmd); // retrieve the first command separator in the string

    if (sep == '0') // no command separators found
    {
        cmd_line = realloc(cmd_line, (lc + 1) * sizeof(command *)); // resize memory block(expand)

        cmd_line[lc] = malloc(sizeof(command)); // allocate memory space

        // NULL the new struct
        cmd_line[lc]->argv = NULL;
        cmd_line[lc]->redirect_in = NULL;
        cmd_line[lc]->redirect_out = NULL;
        cmd_line[lc]->com_name = NULL;
        cmd_line[lc]->pipe_to = 0;
        cmd_line[lc]->background = 0;
        cmd_line[lc]->sequential = 0;

        process_cmd(cmd, cmd_line[lc]); // process command
        lc++;                           // increment counter
    }
    else if (sep == '&') // '&' found (concurrent)
    {
        char *next_cmd;               // stores the next command
        char con_command[CMD_LENGTH]; // string
        strcpy(con_command, cmd);     // copy contents of cmd to temp
        strtok(con_command, "&");     // tokenize the command delimited by '&'

        cmd = (index(cmd, '&')) + 1; // remove tokenised command from cmd
        next_cmd = strtok(cmd, "");  // check for more command arguments

        cmd_line = realloc((void *)cmd_line, (lc + 1) * sizeof(command *)); // resize memory block(expand)
        cmd_line[lc] = calloc(1, sizeof(command));                          // allocate memory and set to null

        process_cmd(con_command, cmd_line[lc]); // create a new command struct
        cmd_line[lc]->background = 1;           // set 'background' value to true
        lc++;                                   // increment counter

        if (next_cmd != NULL)         // if another token exists
            process_cmd_line(cmd, 0); // Process the next Token (recursive)
    }
    else if (sep == '|')
    {
        char *next_cmd;               // stores the next command
        char pip_command[CMD_LENGTH]; // string
        strcpy(pip_command, cmd);     // copy contents of cmd to temp
        strtok(pip_command, "|");     // tokenize the command delimited by '|'

        cmd = (index(cmd, '|')) + 1; // remove tokenised command from cmd
        next_cmd = strtok(cmd, "");  // check for more command arguments

        cmd_line = realloc((void *)cmd_line, (lc + 1) * sizeof(command *)); // resize memory block(expand)
        cmd_line[lc] = calloc(1, sizeof(command));                          // allocate memory and set to null

        process_cmd(pip_command, cmd_line[lc]); // create a new command struct
        cmd_line[lc]->pipe_to = lc + 1;         // set 'pipe_to' value to counter +1
        lc++;                                   // increment counter

        if (next_cmd != NULL)         // if another token exists
            process_cmd_line(cmd, 0); // Process the next Token (recursive)
    }
    else if (sep == ';') // ';' found (sequential)
    {
        char *next_cmd;               // stores the next command
        char seq_command[CMD_LENGTH]; // string
        strcpy(seq_command, cmd);     // copy contents of cmd to temp
        strtok(seq_command, ";");     // tokenize the command delimited by ';'

        cmd = (index(cmd, ';')) + 1; // remove tokenised command from cmd
        next_cmd = strtok(cmd, "");  // check for more command arguments

        cmd_line = realloc((void *)cmd_line, (lc + 1) * sizeof(command *));
        // resize memory block(expand)
        cmd_line[lc] = calloc(1, sizeof(command));
        // allocate memory while nulling each element

        process_cmd(seq_command, cmd_line[lc]); // create a new command struct
        cmd_line[lc]->sequential = 1;           // set 'sequential' value to true
        lc++;                                   // increment counter

        if (next_cmd != NULL)         // if another token exists
            process_cmd_line(cmd, 0); // Process the next Token (recursive)
    }

    cmd_line = realloc((void *)cmd_line, (lc + 1) * sizeof(command *));
    cmd_line[lc] = NULL;
    free(rc_copy);
    return cmd_line;
}

int check_cmd_input(const char *cmd)
{
    // Check if the command is empty
    if (cmd[0] == '\0')
    {
        return 1; // Indicate that the user hit enter without any input
    }

    // Check if the command is too short
    size_t len = strlen(cmd);
    if (len < CMD_MIN_LENGTH)
    {
        return 2; // Indicate that the command has too few arguments
    }

    // // Check if the command begins with whitespace or a command separator
    // if (isspace((unsigned char)cmd[0]) || cmd[0] == '&' || cmd[0] == '|' || cmd[0] == ';')
    // {
    //     return 3; // Indicate an invalid start character
    // }

    // Check for consecutive command separators
    for (size_t i = 0; i < len - 1; ++i)
    {
        if ((cmd[i] == '&' || cmd[i] == '|' || cmd[i] == ';') &&
            (cmd[i] == cmd[i + 1]))
        {
            return 4; // Indicate invalid consecutive separators
        }
    }
    return 0;
}

char lead_separator(const char *cmd)
{
    char sep[3] = {';', '&', '|'};
    int none = 9999;
    int sep_val[3] = {none, none, none};
    int smallest = none;
    char first = '0';

    for (int i = 0; i < 3; i++)
    {
        const char *ptr = strchr(cmd, sep[i]);
        if (ptr)
        {
            int index = (int)(ptr - cmd);
            sep_val[i] = index;
        }
    }
    for (int i = 0; i < 3; i++)
    {
        if (sep_val[i] < smallest) // index value is closer to the left of string
        {
            smallest = sep_val[i]; // assign index value to smallest
            first = sep[i];        // assign separator char to first
        }
    }
    return first;
}

/*
 * This function dumps the contents of the structure to stdout.
 *
 * Arguments :
 *      c - the structure to be displayed.
 *      count - the array position of the structure.
 *
 * Returns :
 *      None.
 *
 */
void dump_structure(command *c, int count)
{
    int lc = 0;

    printf("---- Command(%d) ----\n", count);
    printf("%s\n", c->com_name);
    if (c->argv != NULL)
    {
        while (c->argv[lc] != NULL)
        {
            printf("+-> argv[%d] = %s\n", lc, c->argv[lc]);
            lc++;
        }
    }
    printf("Background = %d\n", c->background);
    printf("Redirect Input = %s\n", c->redirect_in);
    printf("Redirect Output = %s\n", c->redirect_out);
    printf("Pipe to Command = %d\n\n", c->pipe_to);

    return;
}

/*
 * This function dumps the contents of the structure to stdout in a human
 * readable format..
 *
 * Arguments :
 *      c - the structure to be displayed.
 *      count - the array position of the structure.
 *
 * Returns :
 *      None.
 *
 */
void print_human_readable(command *c, int count)
{
    int lc = 1;

    printf("Program : %s\n", c->com_name);
    if (c->argv != NULL)
    {
        printf("Parameters : ");
        while (c->argv[lc] != NULL)
        {
            printf("%s ", c->argv[lc]);
            lc++;
        }
        printf("\n");
    }
    if (c->background == 1)
        printf("Execution in Background.\n");
    if (c->redirect_in != NULL)
        printf("Redirect Input from %s.\n", c->redirect_in);
    if (c->redirect_out != NULL)
        printf("Redirect Output to %s.\n", c->redirect_out);
    if (c->pipe_to != 0)
        printf("Pipe Output to Command# %d\n", c->pipe_to);
    printf("\n\n");

    return;
}