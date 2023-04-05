#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <termcap.h>
#include <iostream>
#include <vector>
#include <dirent.h>
#include <signal.h>
#include <algorithm>
#include <math.h>
#include "colors.hpp"
#include "../util.hpp"
#include "input.hpp"

#define bool int
#define true 1
#define false 0
struct termios term, orig_term;

std::vector<std::string> path_files;

void init_path_files() {
    std::string path = getenv("PATH");
    std::string delimiter = ":";
    size_t pos = 0;
    std::string token;
    while ((pos = path.find(delimiter)) != std::string::npos) {
        token = path.substr(0, pos);

        // Read all files in the directory
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(token.c_str())) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                path_files.push_back(ent->d_name);
            }
            closedir(dir);
        }

        path.erase(0, pos + delimiter.length());
    }

    // Sort by length
    std::sort(path_files.begin(), path_files.end(), [](std::string a, std::string b) { // -std=c++11
        return a.length() < b.length();
    });
}

std::string get_completion(std::string str) {
    for (int i = 0; i < path_files.size(); i++) {
        if (path_files[i].length() >= str.length() && path_files[i].substr(0, str.length()) == str) {
            return path_files[i];
        }
    }
    return "";
}

void enable_input_mode() {
    // Wait for key press instead of enter
    tcgetattr(STDIN_FILENO, &orig_term);
    term = orig_term;
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
}

char get_key() {
    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
}

std::string get_prefix(bool color = true) {
    std::string cwd = getcwd(NULL, 0);
    // Replace home with ~
    char *home = getenv("HOME");
    if (cwd.find(home) == 0) {
        cwd.replace(0, strlen(home), "~");
    }

    return (color ? get_fg_color(100, 200, 255) : "") + cwd + (color ? get_fg_color(100, 255, 200) : "") + " $ " + (color ? RESET : "");
}

int y = 0;
int x = 0;

// Signal
void sigint_input_handler(int a) {
    set_pos(get_prefix(false).length() + x, y - 1);
    std::cout << get_bg_color(255, 255, 255) << get_fg_color(0, 0, 0) << "%c" << RESET;
    set_pos(get_prefix(false).length() + x, y - 1);
    fflush(stdout);
}

std::string get_input() {
    std::string input;
    std::string old_input;
    signal(SIGINT, sigint_input_handler);

    enable_input_mode();
    y = get_y_pos();
    x = 0;

    set_pos(0, y - 1);
    std::cout << get_prefix();
    fflush(stdout);

    while (1) {
        int c = get_key();
        double time1 = get_time();
        
        std::string match = get_completion(input);
        std::string match_substr = "";
        if (match.length() > input.length())
            match_substr = match.substr(input.length() + 1, match.length());

        if (c == 10) break;

        switch (c) {
            case 127:
                if (x > 0) {
                    // Remove the character before x
                    input = input.substr(0, x - 1) + input.substr(x, input.length());
                    x--;
                }
                break;

            case 23:
                if (x > 0) {
                    // Remove the character before x
                    input = input.substr(0, x - 1) + input.substr(x, input.length());
                    x--;

                    // Keep deleting the current word
                    while (x > 0 && input[x - 1] != ' ') {
                        input = input.substr(0, x - 1) + input.substr(x, input.length());
                        x--;
                    }
                }
                break;

            case 27:
                if (get_key() != 91) break;
                switch (get_key()) {
                    case 68:
                        if (x > 0) {
                            x--;
                        }
                        break;
                    case 67:
                        if (x < input.length()) {
                            x++;
                        } else {
                            input = match;
                            x = input.length();
                            match_substr = "";
                        }
                        break;
                }
                break;

            case 9:
                if (match_substr.length() > 0) {
                    input += match_substr;
                    x += match_substr.length();
                    match_substr = "";
                }
                break;

            default:
                if (c >= 32 && c <= 126) {
                    // Insert the character at x
                    input = input.substr(0, x) + (char)c + input.substr(x, input.length());
                    x++;
                    // left arrow
                }
                break;
        }

        set_pos(0, y - 1);
        std::string diff_spaces = "";

        for (int i = 0; i < abs((int)old_input.length() - (int)input.length()); i++) {
            diff_spaces += " ";
        }

        std::string prefix = get_prefix(true);
        std::string prefix_no_c = get_prefix(false);
        std::cout << prefix << get_fg_color(205, 225, 255) << input << get_fg_color(100, 100, 100) << match_substr << diff_spaces << RESET;
        set_pos(prefix_no_c.length() + x, y - 1);
        fflush(stdout);

        old_input = input + match_substr;

        set_pos(0, y);
        double timeRound = round((get_time() - time1) * 1000000);
        char timeConv[1024];
        memset(timeConv, 0, 1024);
        snprintf(timeConv, 1024, "%f", timeRound);
        std::string timeStr = timeConv;
        int len = timeStr.length();
        timeStr = timeStr.substr(0, (len > 5) ? 5 : len);

        // printf("Time: %s", timeStr.c_str());
        set_pos(prefix_no_c.length() + x, y - 1);
        fflush(stdout);
    }

    disable_raw_mode();
    printf("\r\n");

    signal(SIGINT, NULL);

    return input;
}

void init_pos() {
    char buf[1024];
    char *term = getenv("TERM");

    if (term == NULL) {
        printf("TERM environment variable not set.\n");
        exit(1);
    }

    int success = tgetent(buf, term);
    if (success < 0) {
        printf("Could not access the termcap data base.\n");
        exit(1);
    } else if (success == 0) {
        printf("Terminal type %s is not defined.\n", term);
        exit(1);
    }
}

void set_pos(int x, int y) {
    init_pos();

    char *str = tgoto(tgetstr(literal_to_char("cm"), NULL), x, y);
    printf("%s", str);
}

int get_pos(int *y, int *x) {
    char buf[30]={0};
    int ret, i, pow;
    char ch;

    *y = 0; *x = 0;

    struct termios term, restore;

    tcgetattr(0, &term);
    tcgetattr(0, &restore);
    term.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(0, TCSANOW, &term);

    write(1, "\033[6n", 4);

    for( i = 0, ch = 0; ch != 'R'; i++ )
    {
        ret = read(0, &ch, 1);
        if ( !ret ) {
        tcsetattr(0, TCSANOW, &restore);
        fprintf(stderr, "getpos: error reading response!\n");
        return 1;
        }
        buf[i] = ch;
    }

    if (i < 2) {
        tcsetattr(0, TCSANOW, &restore);
        return(1);
    }

    for( i -= 2, pow = 1; buf[i] != ';'; i--, pow *= 10)
        *x = *x + ( buf[i] - '0' ) * pow;

    for( i-- , pow = 1; buf[i] != '['; i--, pow *= 10)
        *y = *y + ( buf[i] - '0' ) * pow;

    tcsetattr(0, TCSANOW, &restore);
    return 0;
}

int get_y_pos() {
    int x, y;
    get_pos(&y, &x);
    return y;
}

int get_x_pos() {
    int x, y;
    get_pos(&y, &x);
    return x;
}

