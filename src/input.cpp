#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <termcap.h>
#include <iostream>
#include "util.hpp"
#include "input.hpp"

#define bool int
#define true 1
#define false 0
struct termios term, orig_term;

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

std::string get_prefix() {
    return ">> ";
}

std::string get_input() {
    std::string input;

    enable_input_mode();
    int y = get_y_pos();

    set_pos(0, y - 1);
    std::cout << get_prefix();
    fflush(stdout);

    while (1) {
        char c = get_key();
        // Start a timer to see how long it takes to register a key

        if (c == 10) {
            break;
        } else if (c == 127) {
            if (input.length() > 0) {
                input.pop_back();
            }
        } else {
            input += c;
        }

        set_pos(0, y - 1);
        std::cout << get_prefix() << input;
        fflush(stdout);
    }

    disable_raw_mode();
    printf("\r\n");

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

