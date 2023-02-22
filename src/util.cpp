
#include <iostream>
#include <string>
#include "util.h"

char *literal_to_char(std::string literal) {
    char *cstr = new char[literal.length() + 1];
    strcpy(cstr, literal.c_str());
    return cstr;
}
