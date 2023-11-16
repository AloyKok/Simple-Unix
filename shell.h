#ifndef SHELL_H
#define SHELL_H

/*
 * Shell.h
 * Header file for shell.c
 * Authors : Aloysious Kok & Gerald
 * Last Update : 15/11/23
 */

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <stddef.h>
#include <glob.h>
#include <termios.h>
#include "parser.h"

/* Constants */
#define MAX_BUF_SIZE 1000
#define MAX_ARRAY_SIZE 500
#define HISTORY_SIZE 100

/* int main(void)
 * This is the main script that will run when running the shell program
 * Sets the signal blockers and start taking in input from stdin
 *
 *
 * Returns :
 *      0 - successful termination of function
 */
int main(void);

/* void setup_signal_handlers()
 * Sets up signal handlers for the shell.
 *
 * No arguments.
 *
 * No return value.
 */
void setup_signal_handlers();

/* void run_shell_loop()
 * Runs the main loop of the shell, handling user input and command execution.
 *
 * No arguments.
 *
 * No return value.
 */
void run_shell_loop();

/* char *read_command_line()
 * Reads a line of input from the user.
 *
 * No arguments.
 *
 * Returns:
 *      A dynamically allocated string containing the input line.
 */
char *read_command_line();

/* int execute_stack (command ** cmd_stack);
 *
 * This function executes the array of command structs passed as an argument.
 * It checks for any builtin commands and executes the builtin commands
 * where required. Depending on the data in the struct, this function will call
 * the different execution functions available
 *
 * Arguments :
 *      cmd_stack - the stack of command structs to be processed.
 *
 * Returns :
 *      0 - successful termination of function
 *
 */
int execute_stack(command **cmd_stack);

/* int exec_sequential (command ** cmd_stack, int current)
 *
 * This function sequentially executes command information stored
 * in the command struct of the specified index and command stack passed in
 * as an argument. It redirects output and inputs where necessary and
 * passes wildcard to the glob function to process it.
 *
 * Arguments :
 *      cmd_stack - the stack of command structs to be processed.
 *	current - the current index in the stack to be processed.
 *
 * Returns :
 *      0 - successfully processed command
 *     -1 - failure to process command
 *
 */
int exec_sequential(command **cmd_stack, int current);

/* int exec_concurrent (command ** cmd_stack, int current)
 *
 * This function concurrently executes command information stored
 * in the command struct of the specified index and command stack passed in
 * as an argument. It redirects output and inputs where necessary and
 * passes wildcard to the glob function to process it.
 *
 * Arguments :
 *      cmd_stack - the stack of command structs to be processed.
 *	current - the current index in the stack to be processed.
 *
 * Returns :
 *      0 - successfully processed command
 *     -1 - failure to process command
 *
 */
int exec_concurrent(command **cmd_stack, int current);

/* int exec_pipe (command ** cmd_stack, int current)
 *
 * This function executes command information stored in the
 * command struct of the specified index and command stack passed in
 * as an argument. It pipes the output of the previous command
 * into the next command. It redirects output and inputs where
 * necessary and passes wildcard to the glob function to process it.
 *
 * Arguments :
 *      cmd_stack - the stack of command structs to be processed.
 *	current - the current index in the stack to be processed.
 *
 * Returns :
 *      0 - successfully processed command
 *     -1 - failure to process command
 *
 */
int exec_pipe(command **cmd_stack, int current);

/* int builtin_menu (command *cmd)
 *
 * This function checks which of the builtin commands is the
 * indexed command in the command stack passed in as arguments.
 * This function calls the corresponding builtin functions.
 *
 * Arguments :
 *      cmd - the command struct to be processed
 *
 * Returns :
 *      1 - processes builtin_cd
 *      2 - processes builtin_pwd
 *	3 - processes builtin_help
 *	4 - processes builtin_prompt
 *	5 - processes builtin_exit
 *     -1 - error in processing builtin functions
 */
int builtin_menu(command *cmd);

/* int builtin_cd (command *cmd)
 *
 * This function checks the passed command struct if there are arguments.
 * If there are arguments: attempt to change directory to the
 * path passed in as an argument.
 * If there are no arguments or '~' or '.' :
 * changes current directory to the "HOME" directory
 * If the arguments are '-' or '--' or '..':
 * goes to the previous path in the current directory
 *
 * Arguments :
 *      cmd - the command struct to be processed
 *
 * Returns :
 *      0 - processes builtin_cd successfully
 *     -1 - error in processing builtin_cd
 */
int builtin_cd(command *cmd);

/* int builtin_prompt(command * cmd)
 *
 * This function checks the passed command struct if there are arguments.
 * If there are arguments: change the current prompt (e.g. default prompt'%')
 * to the argument passed in as the new prompt (e.g. '$')
 * If no argument is passed: return error code of -1
 *
 * Arguments :
 *      cmd - the command struct to be processed
 *
 * Returns :
 *      0 - processes builtin_prompt successfully
 *     -1 - error in processing builtin_prompt
 */
int builtin_prompt(command *cmd);

/* int builtin_pwd()
 *
 * This function prints out the current directory.
 *
 * Returns :
 *      0 - processes builtin_pwd successfully
 *     -1 - error in processing builtin_pwd
 */
int builtin_pwd();

/* int builtin_help()
 *
 * This function prints out the help information for
 * the Simple Unix Shell program.
 *
 * Returns :
 *      0 - processes builtin_help successfully
 */
int builtin_help();

/* int builtin_exit()
 *
 * This function kills the
 * Simple Unix Shell program.
 *
 * Returns :
 *      None
 */
int builtin_exit();

/* int wildcard_handler (command ** cmd_stack, int current)
 *
 * This function checks command information stored in the
 * command struct of the specified index and command stack passed in
 * as an argument. If there are wild card characters in the argument,
 * it is passed on to the glob_t struct globbuf to be processed
 * by the glob function.
 *
 * Arguments :
 *      cmd - the command struct to be processed
 *
 * Returns :
 *      wc_count - the number of wild cards in the given command struct
 */
int wildcard_handler(command **cmd_stack, int current);

/* void claim_zombies()
 *
 * This function claims the zombies processes
 *
 * Returns :
 *      None
 */
void claim_zombies();

#endif
