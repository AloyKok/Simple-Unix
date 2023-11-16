/*
 * Shell.c
 * A Simple Unix Shell
 * Authors : Aloysious Kok & Gerald
 * Last Update :15/11/23
 */

#include "shell.h"

// builtin commands
const char *builtin_cmds[] = {"cd", "pwd", "help", "prompt", "exit", "history"};

// default % prompt string
char prompt_str[MAX_BUF_SIZE] = "% ";

// wild card chars
const char wc_char[] = {'*', '?'};

// glob_t for wildcard handling
glob_t globbuf;

// stores path of last directory visited
char prev_dir[MAX_BUF_SIZE];

// prev_dir_flag = 1 if prev_dir contains a path
int prev_dir_flag = 0;

// current command index position
int curr_idx = 0;

char *command_history[HISTORY_SIZE]; // Array to store history commands
int history_count = 0;               // Counter for the number of commands in history

void add_command_to_history(char *cmd)
{
    if (history_count < HISTORY_SIZE)
    {
        command_history[history_count++] = strdup(cmd);
    }
    else
    {
        free(command_history[0]);
        memmove(command_history, command_history + 1, sizeof(char *) * (HISTORY_SIZE - 1));
        command_history[HISTORY_SIZE - 1] = strdup(cmd);
    }
}

int builtin_history()
{
    for (int i = 0; i < history_count; i++)
    {
        printf("%d: %s\n", i + 1, command_history[i]);
    }
    return 0;
}

void cleanup_history()
{
    for (int i = 0; i < history_count; i++)
    {
        free(command_history[i]);
    }
}

int main(void)
{
    printf("\nSimple Unix Shell.\n\n");

    setup_signal_handlers();
    run_shell_loop();
    cleanup_history(); // Cleanup command history

    return EXIT_SUCCESS;
}

void setup_signal_handlers()
{
    struct sigaction action = {0};
    action.sa_handler = SIG_IGN; // ignore signals

    // Handle SIGTSTP, SIGINT, and SIGQUIT with the same handler
    const int signals_to_ignore[] = {SIGTSTP, SIGINT, SIGQUIT};
    for (size_t i = 0; i < sizeof(signals_to_ignore) / sizeof(signals_to_ignore[0]); i++)
    {
        if (sigaction(signals_to_ignore[i], &action, NULL) != 0)
        {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }
    }

    // Setup handler for SIGCHLD to clean up zombie processes
    struct sigaction sa_child = {0};
    sa_child.sa_handler = claim_zombies;
    sa_child.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa_child, NULL) != 0)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

void run_shell_loop()
{
    char *line = NULL;
    command **cmd_stack = NULL;

    while (1)
    {
        printf("%s", prompt_str);
        line = read_command_line(); // This function will handle the EINTR case internally

        // Check if the command is a history command
        if (line != NULL && line[0] == '!')
        {
            int num = atoi(&line[1]);
            if (num > 0 && num <= history_count)
            {
                free(line); // Free the original line first
                line = strdup(command_history[num - 1]);
                printf("Executing command from history: %s\n", line);
            }
            else
            {
                printf("No such command in history.\n");
                free(line); // Free the line and continue
                continue;
            }
        }

        if (line == NULL)
        {
            continue; // Empty line or read error, just start the loop again
        }

        int cmd_status = check_cmd_input(line);
        if (cmd_status == 0)
        {
            cmd_stack = process_cmd_line(line, 1);
            execute_stack(cmd_stack);
            clean_up(cmd_stack);
        }
        else if (cmd_status == 2)
        {
            // Specific case, possibly handle differently
        }
        else
        {
            printf("Error: command line syntax \n\n");
        }

        free(line); // Ensure we always free the line after we're done with it
    }
}

// The read_command_line function would encapsulate reading from stdin and handling EINTR
// Replace the read_command_line function with a new version
char *read_command_line()
{
    static struct termios oldt, newt;
    int ch, history_index = history_count - 1;
    char *line = (char *)malloc(CMD_LENGTH);
    int position = 0;

    // Save old settings and set new terminal mode
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (1)
    {
        ch = getchar();

        // Handle Ctrl-D (EOF)
        if (ch == EOF)
        {
            clearerr(stdin);            // Clear the EOF condition
            printf("\n%s", prompt_str); // Reprint the prompt
            continue;                   // Continue the loop to read the next line
        }

        // Handle special characters (Ctrl-Z, Ctrl-C, Ctrl-\)
        if (ch == 26 || ch == 3 || ch == 28)
        {
            printf("^%c", ch + 64); // Display ^Z, ^C, ^\ (ASCII art for control characters)
            continue;
        }
        if (ch == '\n' || ch == '\r')
        {
            line[position] = '\0';
            printf("\n");
            break;
        }
        else if (ch == 27) // Arrow key prefix
        {
            getchar(); // Skip '['
            ch = getchar();
            if (ch == 'A' && history_index >= 0)
            { // Up arrow
                strcpy(line, command_history[history_index--]);
                printf("\33[2K\r%s%s", prompt_str, line);
                position = strlen(line);
            }
            else if (ch == 'B' && history_index < history_count - 1)
            { // Down arrow
                strcpy(line, command_history[++history_index]);
                printf("\33[2K\r%s%s", prompt_str, line);
                position = strlen(line);
            }
        }
        else if (ch == 127) // Backspace
        {
            if (position > 0)
            {
                position--;
                line[position] = '\0';
                printf("\b \b");
            }
        }
        else if (position < CMD_LENGTH - 1)
        {
            line[position++] = ch;
            printf("%c", ch);
        }
    }

    // Restore old settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    if (position > 0)
    { // If line is not empty
        add_command_to_history(line);
    }

    return line;
}

int execute_stack(command **cmd_stack)
{
    curr_idx = 0;
    int builtin_exists;

    while (cmd_stack[curr_idx] != NULL)
    {
        // Execute builtin commands if exist
        builtin_exists = 0;
        builtin_exists = builtin_menu(cmd_stack[curr_idx]);
        if (builtin_exists > 0)
        {
            curr_idx++;
            continue;
        }
        else // Other Commmands
        {
            if (cmd_stack[curr_idx]->pipe_to > 0)
            {
                exec_pipe(cmd_stack, curr_idx);
            }
            else if (cmd_stack[curr_idx]->background == 1)
            {
                exec_concurrent(cmd_stack, curr_idx);
            }
            else if (cmd_stack[curr_idx]->sequential == 1)
            {
                exec_sequential(cmd_stack, curr_idx);
            }
            else
            {
                exec_sequential(cmd_stack, curr_idx);
            }
            curr_idx++;
        }
    }
    return 0;
}

int exec_sequential(command **cmd_stack, int current)
{
    pid_t pid;
    pid_t child_pid;
    int status;
    int w_count;

    w_count = wildcard_handler(cmd_stack, current);

    int inputfile = 0;
    int rd_in_flag = 0;
    int outputfile = 0;
    int rd_out_flag = 0;

    // redirect input
    if (cmd_stack[current]->redirect_in != NULL)
    {
        rd_in_flag = 1;
        // read from input file
        if ((inputfile = open(cmd_stack[current]->redirect_in, O_RDONLY)) == -1)
        {
            return -1;
        }
    }
    // redirect output
    else if (cmd_stack[current]->redirect_out != NULL)
    {
        rd_out_flag = 1;
        // write/create a output file with read and execute access
        if ((outputfile = open(cmd_stack[current]->redirect_out, O_WRONLY | O_CREAT, 0755)) == -1)
        {
            return -1;
        }
    }

    // child process
    pid = fork();
    if (pid == 0)
    {
        // redirect to inputfile
        if (rd_in_flag != 0)
        {
            dup2(inputfile, STDIN_FILENO);
        }
        // redirect to outputfile
        else if (rd_out_flag != 0)
        {
            dup2(outputfile, STDOUT_FILENO);
        }

        // if command contains wildcards execute with glob_t
        if (w_count > 0)
        {
            execvp(cmd_stack[current]->argv[0], &globbuf.gl_pathv[0]);
            perror("execvp error: ");
            exit(EXIT_FAILURE);
        }

        // execute regular command
        else
        {
            execvp(cmd_stack[current]->argv[0], cmd_stack[current]->argv);
            perror("execvp error: ");
            exit(EXIT_FAILURE);
        }
    }
    else if (pid < 0)
    {
        perror("Error: ");
    }
    // Parent process
    else
    {
        child_pid = pid;
        waitpid(child_pid, &status, 0);
        if (WIFEXITED(status) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int exec_concurrent(command **cmd_stack, int current)
{

    pid_t pid;
    pid_t child_pid;
    int w_count;

    w_count = wildcard_handler(cmd_stack, current);

    int inputfile = 0;
    int rd_in_flag = 0;
    int outputfile = 0;
    int rd_out_flag = 0;

    // redirect input
    if (cmd_stack[current]->redirect_in != NULL)
    {
        rd_in_flag = 1;
        // read from input file
        if ((inputfile = open(cmd_stack[current]->redirect_in, O_RDONLY)) == -1)
        {
            return -1;
        }
    }

    // redirect output
    else if (cmd_stack[current]->redirect_out != NULL)
    {
        rd_out_flag = 1;
        // write/create a output file with read and execute access
        if ((outputfile = open(cmd_stack[current]->redirect_out, O_WRONLY | O_CREAT, 0755)) == -1)
        {
            return -1;
        }
    }

    // child process
    pid = fork();
    if (pid == 0)
    {
        // setpgid: to put child in new process group allow exec function to run
        setpgid(0, 0);

        // redirect input
        if (rd_in_flag != 0)
        {
            dup2(inputfile, STDIN_FILENO);
        }
        // redirect output
        else if (rd_out_flag != 0)
        {
            dup2(outputfile, STDOUT_FILENO);
        }

        // if command contains wildcards execute with glob_t
        if (w_count > 0)
        {
            execvp(cmd_stack[current]->argv[0], &globbuf.gl_pathv[0]);
            perror("execvp error: ");
            exit(EXIT_FAILURE);
        }

        // execute regular command
        else
        {
            execvp(cmd_stack[current]->argv[0], cmd_stack[current]->argv);
            perror("execvp error: ");
            exit(EXIT_FAILURE);
        }
    }
    else if (pid < 0) // error forking child
    {
        return -1;
    }
    // parent
    else
    {
        child_pid = pid;
        printf("\nbackground process: %d is running\n\n", child_pid);
    }
    return 0;
}

int exec_pipe(command **cmd_stack, int current)
{
    int idx = current;
    int p_count = 0;
    int stdin_desc;
    while (cmd_stack[idx] != NULL && cmd_stack[idx]->pipe_to > 0)
    {
        p_count++;
        idx++;
    }
    // Error handling: Check if there is a valid command after the last pipe
    if (cmd_stack[idx] == NULL)
    {
        fprintf(stderr, "Error: No command after pipe.\n");
        return -1; // Return an error
    }

    idx = current;

    // to prevent segmentation fault
    stdin_desc = dup(0);

    for (int i = 0; i < p_count; i++)
    {
        pid_t pid;
        int w_count;
        int pipefd[2];
        pipe(pipefd);

        w_count = wildcard_handler(cmd_stack, current);

        int inputfile = 0;
        int rd_in_flag = 0;
        int outputfile = 0;
        int rd_out_flag = 0;
        // redirect input
        if (cmd_stack[idx]->redirect_in != NULL)
        {
            rd_in_flag = 1;
            // read from input file
            if ((inputfile = open(cmd_stack[idx]->redirect_in, O_RDONLY)) == -1)
            {
                return -1;
            }
        }
        // redirect output
        else if (cmd_stack[idx]->redirect_out != NULL)
        {
            rd_out_flag = 1;
            // write/create a output file with read and execute access
            if ((outputfile = open(cmd_stack[idx]->redirect_out, O_WRONLY | O_CREAT, 0755)) == -1)
            {
                return -1;
            }
        }

        // child process
        pid = fork();
        if (pid == 0)
        {
            // Write to pipe and close read end
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);

            if (rd_in_flag != 0) // redirect input
            {
                dup2(inputfile, STDIN_FILENO);
            }
            else if (rd_out_flag != 0) // redirect output
            {
                dup2(outputfile, STDOUT_FILENO);
            }

            // if command contains wildcards execute with glob_t
            if (w_count > 0)
            {
                execvp(cmd_stack[idx]->argv[0], &globbuf.gl_pathv[0]);
                perror("execvp error: ");
                exit(EXIT_FAILURE);
            }

            // execute regular command
            else
            {
                execvp(cmd_stack[idx]->argv[0], cmd_stack[idx]->argv);
                perror("execvp error: ");
                exit(EXIT_FAILURE);
            }
        }
        else if (pid < 0) // fork error
        {
            return -1;
        }

        // wait(NULL);//parent waits for child to finish

        // Read from pipe and close write end
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);

        idx++;
    }

    // run last command
    curr_idx = idx;
    if (cmd_stack[curr_idx]->background == 1)
    {
        exec_concurrent(cmd_stack, curr_idx);
    }
    else if (cmd_stack[curr_idx]->sequential == 1)
    {
        exec_sequential(cmd_stack, curr_idx);
    }
    else // no command separator
    {
        exec_sequential(cmd_stack, curr_idx);
    }

    dup2(stdin_desc, 0);
    close(stdin_desc);

    return 0;
}

int builtin_menu(command *cmd)
{
    int builtin_idx = 0;

    // check if cmd->argv[0] is not NULL
    if (cmd != NULL && cmd->argv != NULL && cmd->argv[0] != NULL)
    {
        // check for builtin commands
        for (int i = 0; i < (sizeof(builtin_cmds) / sizeof(char *)); i++)
        {
            if (strcmp(cmd->argv[0], builtin_cmds[i]) == 0)
            {
                builtin_idx = i + 1;
                break;
            }
        }
    }

    switch (builtin_idx)
    {
    case 1:
        if (builtin_cd(cmd) < 0)
            return -1;
        break;
    case 2:
        if (builtin_pwd() < 0)
            return -1;
        break;
    case 3:
        if (builtin_help() < 0)
            return -1;
        break;
    case 4:
        builtin_prompt(cmd);
        break;
    case 5:
        builtin_exit();
        break;
    case 6:
        builtin_history();
        break;
    default:
        break;
    }
    return builtin_idx;
}

int builtin_cd(command *cmd)
{
    char cwd[MAX_BUF_SIZE];
    char path[MAX_BUF_SIZE] = {0};

    // Concatenate arguments if directory name contains spaces
    if (cmd->argv[1] != NULL)
    {
        for (int i = 1; cmd->argv[i] != NULL; i++)
        {
            strncat(path, cmd->argv[i], sizeof(path) - strlen(path) - 1);
            if (cmd->argv[i + 1] != NULL)
                strncat(path, " ", sizeof(path) - strlen(path) - 1);
        }
    }

    // Handling different cd commands
    if (strlen(path) == 0 || strcmp(path, "~") == 0 || strcmp(path, ".") == 0)
    {
        // Go to home directory
        prev_dir_flag = 1;
        getcwd(prev_dir, MAX_BUF_SIZE);
        if (chdir(getenv("HOME")) != 0)
        {
            perror("cd");
            return -1;
        }
    }
    else if (strcmp(path, "-") == 0)
    {
        // Go to previous directory
        if (prev_dir_flag)
        {
            if (chdir(prev_dir) != 0)
            {
                perror("cd");
                return -1;
            }
        }
        else
        {
            printf("previous directory not found\n");
        }
    }
    else
    {
        // Go to specified path
        prev_dir_flag = 1;
        getcwd(prev_dir, MAX_BUF_SIZE);
        if (chdir(path) != 0)
        {
            perror("cd");
            return -1;
        }
    }

    // Print the new current directory
    getcwd(cwd, MAX_BUF_SIZE);
    printf("current directory: %s\n", cwd);
    return 0;
}


int builtin_prompt(command *cmd)
{
    char new_prompt[MAX_BUF_SIZE] = {0};

    if (cmd->argv[1] == NULL)
    {
        printf("Error: prompt cannot be empty\n\n");
        return -1;
    }

    // Concatenate all arguments to form the new prompt
    int space_left = sizeof(new_prompt) - 2; // Reserve 2 bytes (for space and null terminator)
    for (int i = 1; cmd->argv[i] != NULL && space_left > 0; i++)
    {
        // Check for special characters
        if (strchr(cmd->argv[i], '&') || strchr(cmd->argv[i], '|') || strchr(cmd->argv[i], ';'))
        {
            printf("Error: prompt cannot contain '&' or '|' or ';'\n\n");
            return -1;
        }

        int len = snprintf(new_prompt + strlen(new_prompt), space_left, "%s%s",
                           cmd->argv[i], cmd->argv[i + 1] ? " " : "");
        space_left -= len;
    }

    // Add a space at the end of the prompt
    if (space_left > 0)
    {
        strcat(new_prompt, " ");
    }

    // Update prompt
    strncpy(prompt_str, new_prompt, MAX_BUF_SIZE - 1);
    prompt_str[MAX_BUF_SIZE - 1] = '\0'; // Ensure null termination

    printf("\n");
    return 0;
}

int builtin_pwd()
{
    char cwd[MAX_BUF_SIZE];

    // get current directory and copy to cwd
    getcwd(cwd, MAX_BUF_SIZE);

    if (strlen(cwd) != 0)
    {
        printf("%s\n", cwd);
    }
    else
    {
        perror("cwd");
        return -1;
    }
    return 0;
}

int builtin_help()
{
    printf("\nSimple Unix Shell - Help\n");
    printf("--------------------------------------------------------------------------------\n");
    printf("This shell supports a variety of built-in commands. Below is a list of these\n");
    printf("commands along with a brief description of their functionality.\n\n");

    printf("Command Syntax:\n");
    printf("cd [directory]\n");
    printf("    Changes the current directory to the specified path. If no argument is\n");
    printf("    provided, it defaults to the home directory. Usage examples:\n");
    printf("    cd /path/to/directory\n");
    printf("    cd ~ (Navigates to the home directory)\n");
    printf("    cd - (Navigates to the previous directory)\n\n");

    printf("prompt [string]\n");
    printf("    Sets the shell prompt to the specified string. Special characters are not\n");
    printf("    allowed. Example usage: prompt myshell> \n\n");

    printf("pwd\n");
    printf("    Displays the current working directory.\n\n");

    printf("exit\n");
    printf("    Exits the Simple Unix Shell. No arguments required.\n\n");

    printf("--------------------------------------------------------------------------------\n");
    printf("For more information on each command, refer to the assignment documentation\n");

    return 0;
}

int builtin_exit()
{
    printf("\nExiting Simple Unix Shell..\n");
    exit(EXIT_SUCCESS);
}

int wildcard_handler(command **cmd_stack, int current)
{
    int wc_pos[128];      // stores index positions of wildcards in argv
    int wc_pos_count = 0; // position in wc_pos array
    int arg_index = 0;    // argv position index
    int wc_count = 0;     // total number of wild cards

    // Check if cmd_stack[current] and cmd_stack[current]->argv are not NULL
    if (cmd_stack[current] == NULL || cmd_stack[current]->argv == NULL)
    {
        // fprintf(stderr, "Error: Null command or arguments.\n");
        return -1;
    }

    // Store array of indexes in argument wildcards are detected
    while (cmd_stack[current]->argv[arg_index] != NULL)
    {
        char *cp = cmd_stack[current]->argv[arg_index];

        for (int i = 0; i < (sizeof(wc_char) / sizeof(wc_char[0])); i++)
        {
            if (strchr(cp, wc_char[i]))
            {
                // replace wild card char with '*'
                for (int j = 0; j < strlen(cp); j++)
                {
                    if (cp[j] == wc_char[i])
                    {
                        cp[j] = '*';
                    }
                }
                // store argv index position of first wildcard
                wc_pos[wc_pos_count] = arg_index;
                wc_pos_count++;
                wc_count++;
            }
        }
        arg_index++;
    }

    // if wildcard exists, initialise globbuf
    if (wc_count > 0)
    {
        globbuf.gl_offs = wc_count;
        // initialise first wildcard
        if (glob(cmd_stack[current]->argv[wc_pos[0]], GLOB_DOOFFS, NULL, &globbuf) == GLOB_NOMATCH)
        {
            return wc_count = 0;
        }

        // append subsequent wildcards
        for (int i = 1; i < wc_pos_count; i++)
        {
            glob(cmd_stack[current]->argv[wc_pos[i]], GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);
        }

        int is_wc = 0;
        int gl_pathv_pos = 0;
        // for each command argument check if there is a wildcard
        for (int i = 0; i < arg_index; i++)
        {
            for (int j = 0; j < wc_pos_count; j++)
            {
                if (i == wc_pos[j])
                {
                    is_wc = 1;
                }
            }

            // if argument is not a wildcard insert to gl_pathv
            if (is_wc != 1)
            {
                globbuf.gl_pathv[gl_pathv_pos] = cmd_stack[current]->argv[i];
                gl_pathv_pos++;
            }
            is_wc = 0;
        }
    }
    return wc_count;
}

// Function to clean up (reap) zombie child processes.
void claim_zombies()
{
    pid_t pid;
    int status;

    // Repeatedly call waitpid with WNOHANG until no more zombies are found
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        // log the PID and status of reaped children
        printf("Reaped child process with PID: %d, Status: %d\n", pid, status);
    }

    // If waitpid returns -1 and errno is set to ECHILD (there are no child processes)
    if (pid == -1 && errno != ECHILD)
    {
        perror("waitpid failed");
    }
}
