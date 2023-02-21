#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_INPUT 1000
#define MAX_ARGS 100

#define clearscr() printf("\033[H\033[J")

int check_integrated_command(char **parsed);

void init_shell() {
    clearscr();
    sleep(1);
    clearscr();
}

int get_input(char *input) {
    char *buf;
    buf = readline(">>> ");
    if (strlen(buf) != 0) {
        add_history(buf);
        strcpy(input, buf);
        return 0;
    } else {
        return 1;
    }
}

void exec_args(char **parsed) {
    pid_t pid = fork();
    if (pid == -1) {
        printf("Failed to fork child process\n");
        return;
    } else if (pid == 0) {
        if (execvp(parsed[0], parsed) < 0) {
            printf("Failed to execute command.\n");
        }
        exit(0);
    } else {
        wait(NULL);
        return;
    }
}

void exec_args_piped(char **parsed, char **parsed_pip) {
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) < 0) {
        printf("\nPipe could not be initialized\n");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("\nCould not fork\n");
        return;
    }

    if (p1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(parsed[0], parsed) < 0) {
            printf("\nCould not execute command 1..");
            exit(0);
        }
    } else {
        p2 = fork();

        if (p2 < 0) {
            printf("\nCould not fork\n");
            return;
        }

        if (p2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(parsed_pip[0], parsed_pip) < 0) {
                printf("\nCould not execute command 2..");
                exit(0);
            }
        } else {
            wait(NULL);
            wait(NULL);
        }
    }
}

int parse_pipe(char *str, char **str_piped) {
    int i;
    for (i = 0; i < 2; i++) {
        str_piped[i] = strsep(&str, "|");
        if (str_piped[i] == NULL)
            break;
    }

    if (str_piped[1] == NULL)
        return 0;
    else
        return 1;
}

void parse_space(char *str, char **parsed) {
    int i;
    for (i = 0; i < MAX_ARGS; i++) {
        parsed[i] = strsep(&str, " ");
        if (parsed[i] == NULL)
            break;
        if (strlen(parsed[i]) == 0)
            i--;
    }
}

int process_string(char *str, char **parsed, char **parsed_pip) {
    // Replace all variables ($var) with their values
    int i;
    for (i = 0; i < strlen(str); i++) {
        if (str[i] == '$') {
            int j = i + 1;
            char var[100];
            int k = 0;
            while (str[j] != ' ' && str[j] != '\0') {
                var[k] = str[j];
                j++;
                k++;
            }
            var[k] = '\0';

            char *value = getenv(var);
            char *new_str = malloc(strlen(str) + (value == NULL ? 0 : strlen(value)) + 1);
            if (new_str == NULL) {
                printf("Error allocating memory\n");
                exit(1);
            }
            strcpy(new_str, str);
            new_str[i] = '\0';
            if (value != NULL) {
                strcat(new_str, value);
            }
            strcat(new_str, str + j);
            strcpy(str, new_str);
            free(new_str);

        }
    }

    char *str_piped[2];
    int piped = 0;

    piped = parse_pipe(str, str_piped);
    
    if (piped) {
        parse_space(str_piped[0], parsed);
        parse_space(str_piped[1], parsed_pip);
    } else {
        parse_space(str, parsed);
    }

    if (check_integrated_command(parsed))
        return 0;
    else
        return 1 + piped;
}

int check_integrated_command(char **parsed) {
    int no_of_integrated_commands = 3, i, switch_integrated_command = 0;
    char *integrated_commands[no_of_integrated_commands];
    integrated_commands[0] = "cd";
    integrated_commands[1] = "help";
    integrated_commands[2] = "exit";

    for (i = 0; i < no_of_integrated_commands; i++) {
        if (strcmp(parsed[0], integrated_commands[i]) == 0) {
            switch_integrated_command = i + 1;
            break;
        }
    }

    switch (switch_integrated_command) {
        case 1:
            chdir(parsed[1]);
            return 1;
        case 2:
            printf("NSH by @Nebula-Developer ~ 2023\n");
            return 1;
        case 3:
            exit(0);
        default:
            break;
    }

    return 0;
}

int main() {
    char input[MAX_INPUT], *parsed_args[MAX_ARGS];
    char *parsed_args_piped[MAX_ARGS];
    int exec_flag = 0;

    init_shell();

    while (1) {
        if (get_input(input))
            continue;
        exec_flag = process_string(input, parsed_args, parsed_args_piped);

        if (exec_flag == 1)
           exec_args(parsed_args);

        if (exec_flag == 2)
           exec_args_piped(parsed_args, parsed_args_piped);
    }

    return 0;
}
