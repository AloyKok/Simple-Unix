/*
 * Parser.h
 * Data structures and various defines for parser.c
 * Author : Aloysious & Gerald
 * Last Update : 
 */

#ifndef _PARSER_H
#define _PARSER_H

#include <glob.h> // for glob_t

#define CMD_LENGTH 100000
#define CMD_MIN_LENGTH 2
#define MIN_ARGUMENTS 1000

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


// Function prototypes
char *trim_whitespace(char *str);
char *safe_strdup(const char *str);
void *safe_realloc(void *ptr, size_t size);
void free_command(command *cmd);
void process_simple_cmd(char *cmd, command *result);
void process_cmd(char *cmd, command *result);
void clean_up(command **cmd_stack, int cmd_count);
command **process_cmd_line(char *cmd_line, int new);
int check_cmd_input(const char *cmd);
char lead_separator(const char *cmd);
void dump_structure(command *c, int count);
void print_human_readable(command *c, int count);

#endif // PARSER_H