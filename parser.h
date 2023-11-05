#ifndef _PARSER_H
#define _PARSER_H

/*
 * Parser.h
 * Data structures and various defines for parser.c
 * Author : Aloysious Kok & Gerald
 * Last Update : 
 */

#include <glob.h> // for glob_t

#define CMD_LENGTH 100000
#define CMD_MIN_LENGTH 2
#define MIN_ARGUMENTS 1000
#define MIN_COMMANDS 100

#define NUM_SEPARATORS 3
#define NO_SEPARATOR '0'

// Define the command structure
typedef struct Command_struct {
    char *com_name;        // Command name
    char **argv;           // Arguments list
    int background;        // Background execution flag
    int sequential;        // Sequential execution flag
    char *redirect_in;     // Input redirection file
    char *redirect_out;    // Output redirection file
    int pipe_to;           // Piping to another command flag
    glob_t globbuf;        // For wildcard handling
} command;

//Helper Functions


// Function prototypes
command **process_cmd_line(char *cmd_line, int new);
void process_cmd(char *cmd, command *result);
void process_simple_cmd(char *cmd, command *result);
void clean_up(command **cmd_stack, int cmd_count);
int check_cmd_input(const char *cmd);
char lead_separator(const char *cmd);

#endif // PARSER_H