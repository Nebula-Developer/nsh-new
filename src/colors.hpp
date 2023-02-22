
#ifndef _COLORS_HPP_
#define _COLORS_HPP_

#define RESET "\033[0m"
#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"

std::string get_fg_color(int r, int g, int b);
std::string get_bg_color(int r, int g, int b);

#endif