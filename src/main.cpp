#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termcap.h>
#include <iostream>
#include <regex>
#include "util.hpp"
#include "./graphics/input.hpp"

#define bool int
#define true 1
#define false 0

#ifndef MAX_INPUT
#define MAX_INPUT 1000
#endif

#ifndef MAX_ARGS
#define MAX_ARGS 100
#endif

#define clearscr() printf("\033[H\033[J")

bool debug = false;

void debug_log(std::string header, int num) {
    if (debug) printf("%s: %d\n", header.c_str(), num);
}

std::string run_regex(std::string str, std::string regex) {
    std::regex re(regex);
    std::smatch match;
    std::regex_search(str, match, re);
    return match[0];
}

int check_integrated_command(char **parsed, char *all);

void init_shell() {

}

void exec_args(char **parsed) {
    pid_t pid = fork();
    if (pid == -1) {
        printf("nsh: failed to fork\n");
        return;
    } else if (pid == 0) {
        if (execvp(parsed[0], parsed) < 0) {
            printf("nsh: command not found: %s\n", parsed[0]);
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
        printf("\nnsh: Pipe could not be initialized\n");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("\nnsh: Could not fork\n");
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

void parse_space(char* str, char** parsed) {
    debug_log("Parse Space", 1);
    // If str without spaces is empty, return
    std::string str_without_spaces = run_regex(str, "[^ ]+");
    if (str_without_spaces.length() == 0) {
        parsed[0] = NULL;
        return;
    }

    debug_log("Parse Space", 2);

    int i;
    for (i = 0; i < MAX_ARGS; i++) {
        parsed[i] = strsep(&str, " ");
        if (parsed[i] == NULL)
            break;
        if (strlen(parsed[i]) == 0)
            i--;
    }

    debug_log("Parse Space", 3);
}

bool is_exitting_char(char c) {
    return !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));
}

int process_string(char *str, char **parsed, char **parsed_pip) {
    // Replace all variables ($var) with their values
    int i;
    debug_log("Process", 1);
    for (i = 0; i < strlen(str); i++) {
        if (str[i] == '$') {
            int j = i + 1;
            char var[100];
            int k = 0;
            while (!is_exitting_char(str[j])) {
                var[k] = str[j];
                j++;
                k++;
            }
            var[k] = '\0';

            char *value = getenv(var);
            char *new_str = (char *)malloc(strlen(str) + (value == NULL ? 0 : strlen(value)) + 1);
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
        } else if (str[i] == '~') {
            char *home = getenv("HOME");
            char *new_str = (char *)malloc(strlen(str) + strlen(home) + 1);
            if (new_str == NULL) {
                printf("Error allocating memory\n");
                exit(1);
            }
            strcpy(new_str, str);
            new_str[i] = '\0';
            strcat(new_str, home);
            strcat(new_str, str + i + 1);
            strcpy(str, new_str);
            free(new_str);
        }
    }

    debug_log("Process", 2);

    char *str_clone = (char *)malloc(strlen(str));
    strcpy(str_clone, str);

    char *str_piped[2];
    int piped = 0;

    piped = parse_pipe(str, str_piped);
    
    if (piped) {
        parse_space(str_piped[0], parsed);
        parse_space(str_piped[1], parsed_pip);
    } else {
        parse_space(str, parsed);
    }

    debug_log("Process", 3);

    if (check_integrated_command(parsed, str_clone))
        return 0;
    else
        return 1 + piped;
}

int check_integrated_command(char **parsed, char *all) {
    debug_log("Check integrated", 1);
    int no_of_integrated_commands = 4, i, switch_integrated_command = 0;
    std::string integrated_commands[no_of_integrated_commands];
    integrated_commands[0] = "cd";
    integrated_commands[1] = "help";
    integrated_commands[2] = "exit";
    integrated_commands[3] = "export";

    debug_log("Check integrated", 2);

    if (parsed[0] == NULL)
        return 1;

    for (i = 0; i < no_of_integrated_commands; i++) {
        if (strcmp(parsed[0], integrated_commands[i].c_str()) == 0) {
            switch_integrated_command = i + 1;
            break;
        }
    }

    debug_log("Check integrated", 3);

    switch (switch_integrated_command) {
        case 1:
            chdir(parsed[1]);
            setenv("PWD", getcwd(NULL, 0), 1);
            return 1;
        case 2:
            printf("NSH by @Nebula-Developer ~ 2023\n");
            return 1;
        case 3:
            exit(0);
        case 4:
            setenv(parsed[1], all + (strlen(parsed[1]) + strlen(parsed[0]) + 2), 1);
            return 1;
        default:
            break;
    }

    debug_log("Check integrated", 4);

    return 0;
}



int main() {
    init_path_files();
    char input[MAX_INPUT], *parsed_args[MAX_ARGS];
    char *parsed_args_piped[MAX_ARGS];
    int exec_flag = 0;

    init_shell();

    while (1) {
        strcpy(input, get_input().c_str());
        if (strcmp(input, "") == 0) continue;
        exec_flag = process_string(input, parsed_args, parsed_args_piped);

        if (exec_flag == 1) exec_args(parsed_args);

        if (exec_flag == 2) exec_args_piped(parsed_args, parsed_args_piped);
    }

    return 0;
}
