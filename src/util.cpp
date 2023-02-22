
#include <iostream>
#include <string>
#include <cstring>
#include <sys/time.h>
#include "util.hpp"

char *literal_to_char(std::string literal) {
    char *cstr = new char[literal.length() + 1];
    strcpy(cstr, literal.c_str());
    return cstr;
}

double get_time() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
