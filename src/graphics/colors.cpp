#include <iostream>

std::string get_fg_color(int r, int g, int b) {
    return "\x1b[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

std::string get_bg_color(int r, int g, int b) {
    return "\x1b[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}
