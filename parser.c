/*
 * Parser.c
 * A simple Command Line Parser.
 * Author : Michael Roberts <mroberts@it.net.au>
 * Edited by: Aloysious Kok & Gerald
 * Last Modification : 09/11/23
 */

#include "parser.h"

// #define DEBUG

/*
 * This function breakes the simple command token isolated in other functions
 * into a sequence of arguments. Each argument is bounded by white-spaces, and
 * there is no special character intepretation. The results are stored in the
 * argv array of the result command structure.
 *
 * Arguments :
 *      cmd - the string to be processed.
 *      result - the comand struct to store the results in.
 *
 * Returns :
 *      None.
 *
 */
void process_simple_cmd(char *cmd, command *result)
{
   if (!cmd || !result)
   {
      fprintf(stderr, "Invalid input to process_simple_cmd\n");
      return;
   }

   // Initialize command structure members
   *result = (command){0}; // Zero out the entire structure first

   char *token;
   int arg_count = 0;
   size_t arg_size = 0;

   token = strtok(cmd, white_space);
   if (token == NULL) // Handle empty command case
   {
      result->argv = calloc(1, sizeof(char *)); // Allocate space for a NULL pointer
      if (!result->argv)
      {
         fprintf(stderr, "Memory allocation failed in process_simple_cmd\n");
         return;
      }
      result->argv[0] = NULL; // Set the first element to NULL for an empty command
      return;
   }

   while (token != NULL)
   {
      char **temp_argv = realloc(result->argv, (arg_size + 2) * sizeof(char *));

      // When memory allocation fails, call clean_up_single instead of clean_up
      if (!temp_argv)
      {
         fprintf(stderr, "Memory allocation failed in process_simple_cmd\n");
         clean_up_single(result); // Use the new clean_up_single function
         return;
      }

      result->argv = temp_argv;
      result->argv[arg_count] = strdup(token);
      if (!result->argv[arg_count])
      {
         fprintf(stderr, "Memory allocation failed for argument in process_simple_cmd\n");
         clean_up_single(result); // Use the new clean_up_single function
         return;
      }

      if (arg_count == 0)
      {
         result->com_name = result->argv[0];
      }

      arg_count++;
      arg_size++;
      token = strtok(NULL, white_space);
   }

   // Ensure the last element of argv is NULL
   result->argv[arg_count] = NULL;
}

/*
 * This function parses the commands isolated from the command line string in
 * other functions. It searches the string looking for input and output
 * redirection characters. The simple commands found are sent to
 * process_simple_comd(). The redirection information is stored in the result
 * command structure.
 *
 * Arguments :
 *      cmd - the command string to be processed.
 *      result - the command structure to store the results in.
 *
 * Returns :
 *      None.
 *
 */
void process_cmd(char *cmd, command *result)
{
   char *input_redirect_ptr, *output_redirect_ptr;
   char *simple_cmd = NULL;
   char *input_part = NULL;
   char *output_part = NULL;

   // Initialize result structure
   *result = (command){0};

   // Check for redirections
   input_redirect_ptr = strchr(cmd, '<');
   output_redirect_ptr = strchr(cmd, '>');

   if (input_redirect_ptr && output_redirect_ptr)
   {
      // Handle case when both input and output redirections are present
      *input_redirect_ptr = '\0';
      *output_redirect_ptr = '\0';
      input_redirect_ptr++;
      output_redirect_ptr++;

      // Trim leading white spaces
      input_redirect_ptr = strtok(input_redirect_ptr, white_space);
      output_redirect_ptr = strtok(output_redirect_ptr, white_space);

      // Additional error handling for strtok returns
      if (!input_redirect_ptr || !output_redirect_ptr)
      {
         fprintf(stderr, "Syntax error in redirection paths\n");
         return;
      }

      // Split command into three parts
      simple_cmd = strdup(cmd);
      input_part = strdup(input_redirect_ptr);
      output_part = strdup(output_redirect_ptr);

      // Error handling for memory allocation failures
      if (!simple_cmd || !input_part || !output_part)
      {
         fprintf(stderr, "Memory allocation failed in process_cmd\n");
         free(simple_cmd);
         free(input_part);
         free(output_part);
         return;
      }

      // Process the simple command
      process_simple_cmd(simple_cmd, result);

      // Store redirection paths
      result->redirect_in = input_part;
      result->redirect_out = output_part;
   }
   else if (input_redirect_ptr)
   {
      // Check for input redirection
      *input_redirect_ptr = '\0'; // Split the string at '<'
      input_redirect_ptr++;       // Move past the '<' character

      // Trim leading white spaces
      input_redirect_ptr = strtok(input_redirect_ptr, white_space);
      if (!input_redirect_ptr)
      {
         fprintf(stderr, "Syntax error in input redirection path\n");
         return;
      }

      simple_cmd = strdup(cmd);
      if (!simple_cmd)
      {
         fprintf(stderr, "Memory allocation failed for simple command\n");
         return;
      }
      process_simple_cmd(simple_cmd, result);
      result->redirect_in = strdup(input_redirect_ptr);
      if (!result->redirect_in)
      {
         fprintf(stderr, "Memory allocation failed for input redirection\n");
         free(simple_cmd);
         return;
      }
   }
   else if (output_redirect_ptr)
   {
      // Check for output redirection
      *output_redirect_ptr = '\0'; // Split the string at '>'
      output_redirect_ptr++;       // Move past the '>' character

      // Trim leading white spaces
      output_redirect_ptr = strtok(output_redirect_ptr, white_space);
      if (!output_redirect_ptr)
      {
         fprintf(stderr, "Syntax error in output redirection path\n");
         return;
      }

      simple_cmd = strdup(cmd);
      if (!simple_cmd)
      {
         fprintf(stderr, "Memory allocation failed for simple command\n");
         return;
      }
      process_simple_cmd(simple_cmd, result);
      result->redirect_out = strdup(output_redirect_ptr);
      if (!result->redirect_out)
      {
         fprintf(stderr, "Memory allocation failed for output redirection\n");
         free(simple_cmd);
         return;
      }
   }
   else
   {
      // If no redirections were found
      process_simple_cmd(cmd, result);
   }

   // Free the temporary simple_cmd buffer if it was used
   if (simple_cmd)
   {
      free(simple_cmd);
   }
}

int detect_multiple_redirections(char *cmd)
{
   int redirection_count = 0;
   char *current_char = cmd;

   while (*current_char != '\0')
   {
      if (*current_char == '>' || *current_char == '<')
      {
         redirection_count++;
         if (redirection_count > 1)
         {
            // Check if next character is also a redirection operator
            if (*(current_char + 1) == '>' || *(current_char + 1) == '<')
            {
               return 1; // Found consecutive redirection operators
            }
            redirection_count = 0; // Reset count if next char is not redirection
         }
      }
      else
      {
         redirection_count = 0; // Reset count for non-redirection characters
      }
      current_char++;
   }
   return 0; // No consecutive redirection operators found
}

/*
 * This function processes the command line. It isolates tokens seperated by
 * the '&' or the '|' character. The tokens are then passed on to be processed
 * by other functions. Once the first token has been isolated this function is
 * called recursivly to process the rest of the command line. Once the entire
 * command line has been processed an array of command structures is created
 * and returned.
 *
 * Arguments :
 *      cmd - the command line to be processed.
 *
 * Returns :
 *      An array of pointers to command structures.
 *
 */
command **process_cmd_line(char *cmd, int new)
{
   static command **cmd_line = NULL;
   static int lc = 0;
   char sep;

   // Reset static variables when processing a new command line
   if (new)
   {
      // free_cmd_line(cmd_line); // Free any existing command line
      cmd_line = NULL;
      lc = 0;
   }

   // Get the leading separator
   sep = lead_separator(cmd);

   // No separators found, process the whole command line as a single command
   if (sep == '0')
   {
      cmd_line = realloc(cmd_line, (lc + 2) * sizeof(command *));
      if (!cmd_line)
      {
         fprintf(stderr, "Memory allocation failed\n");
         return NULL;
      }

      cmd_line[lc] = calloc(1, sizeof(command));
      if (!cmd_line[lc])
      {
         fprintf(stderr, "Memory allocation failed\n");
         return NULL;
      }

      process_cmd(cmd, cmd_line[lc++]);
   }
   else
   {
      char *next_cmd;
      char *current_cmd = strtok(cmd, &sep);
      next_cmd = strtok(NULL, "");

      if (current_cmd)
      {
         cmd_line = realloc(cmd_line, (lc + 2) * sizeof(command *));
         if (!cmd_line)
         {
            fprintf(stderr, "Memory allocation failed\n");
            return NULL;
         }

         cmd_line[lc] = calloc(1, sizeof(command));
         if (!cmd_line[lc])
         {
            fprintf(stderr, "Memory allocation failed\n");
            return NULL;
         }

         process_cmd(current_cmd, cmd_line[lc]);

         // Set the appropriate flag based on the separator found
         switch (sep)
         {
         case '&':
            cmd_line[lc]->background = 1;
            break;
         case '|':
            cmd_line[lc]->pipe_to = lc + 1;
            break;
         case ';':
            cmd_line[lc]->sequential = 1;
            break;
         }
         lc++;
      }

      // Recursive call if there's another command to process
      if (next_cmd)
      {
         process_cmd_line(next_cmd, 0);
      }
   }

   // Terminate the array with a NULL pointer
   cmd_line = realloc(cmd_line, (lc + 1) * sizeof(command *));
   if (!cmd_line)
   {
      fprintf(stderr, "Memory allocation failed\n");
      return NULL;
   }
   cmd_line[lc] = NULL;

   return cmd_line;
}

// Define a new function to clean up a single command
void clean_up_single(command *cmd)
{
   if (cmd == NULL)
      return;

   int ilpc = 0;
   if (cmd->argv != NULL)
   {
      while (cmd->argv[ilpc] != NULL)
      {
         free(cmd->argv[ilpc]); // Free each pointer in argv
         ilpc++;
      }
      free(cmd->argv); // Free argv itself
   }

   if (cmd->redirect_in != NULL)
   {
      free(cmd->redirect_in); // Free redirect_in
   }
   if (cmd->redirect_out != NULL)
   {
      free(cmd->redirect_out); // Free redirect_out
   }
   free(cmd);
}

void free_cmd_line(command **cmd_line)
{
   if (!cmd_line)
   {
      return;
   }

   int i = 0;
   while (cmd_line[i] != NULL)
   {
      clean_up_single(cmd_line[i]);
      i++;
   }
   free(cmd_line);
}

/*
 * This function cleans up some of the dynamicly allocated memory. Each array
 * element is visited, and the contained data is free'd before the entire
 * structure is free'd.
 *
 * Arguments :
 *      cmd - the array of pointers to command structures to be cleaned.
 *
 * Returns :
 *      None.
 *
 */
void clean_up(command **cmd)
{
   int lpc = 0;
   if (cmd == NULL)
      return;

   while (cmd[lpc] != NULL)
   {
      clean_up_single(cmd[lpc]); // Clean up each command using the new function
      lpc++;
   }
   free(cmd); // Free the array of command pointers
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
} /*End of dump_structure() */

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
} /*End of print_human_readable() */

/* Start of check_cmd_input */
/*
 * This function checks for errors in the user cmd input
 *
 * Arguments :
 *      *cmd - the input from stdin
 *
 * Returns :
 *      0 - No errors
 *	1 - Incomplete arguments or starting with a seperator character
 *	2 - Enter is keyed in
 */
int check_cmd_input(char *cmd)
{
   int value = 0;

   // user hits enter without any input
   if (cmd[0] == '\0')
   {
      value = 2;
   }
   // command has too few args
   else if (strlen(cmd) < MIN_LENGTH)
   {
      value = 1;
   }
   // command begins with white space or a command separator
   else if (cmd[0] == ' ' || cmd[0] == '&' || cmd[0] == '|' || cmd[0] == ';')
   {
      value = 1;
   }
   else
   {
      int ln = strlen(cmd);

      for (int i = 0; i < ln; i++)
      {
         if (cmd[i] == '&' || cmd[i] == '|' || cmd[i] == ';')
         {
            if (cmd[i + 1] == '&' || cmd[i + 1] == '|' || cmd[i + 1] == ';')
               value = 1;
         }
      }
   }
   return value;
}
/* End of check_cmd_input */

/* Start of lead_separator*/
/*
 * This function checks if character in input is a seperator character
 * and returns the first seperator character
 *
 * Arguments :
 *      *cmd - the input from stdin
 *
 * Returns :
 *      0 - No errors
 *	1 - Incomplete arguments or starting with a seperator character
 *	2 - Enter is keyed in
 */
char lead_separator(const char *cmd)
{
   char separators[] = {';', '&', '|', '\0'}; // Include the null terminator to end the array

   for (int i = 0; cmd[i] != '\0'; ++i)
   {
      for (int j = 0; separators[j] != '\0'; ++j)
      {
         if (cmd[i] == separators[j])
         {
            return cmd[i]; // Return the first encountered separator
         }
      }
   }

   return '0';
}