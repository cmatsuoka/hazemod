#include "util/util.h"
#include <cstdarg>
#include <cstring>
#include <string>
#include <memory>


// From https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf#comment61134428_2342176

std::string string_format(std::string const& fmt, ...)
{
    int final_n, n = ((int)fmt.size()) * 2;  // Reserve two times as much as the length of the fmt
    std::unique_ptr<char[]> formatted;
    va_list ap;

    while (true) {
        formatted.reset(new char[n]);        // Wrap the plain char array into the unique_ptr
        strcpy(&formatted[0], fmt.c_str());

        va_start(ap, fmt);
        final_n = vsnprintf(&formatted[0], n, fmt.c_str(), ap);
        va_end(ap);

        if (final_n < 0 || final_n >= n) {
            n += abs(final_n - n + 1);
        } else {
            break;
        }
    }

    return std::string(formatted.get());
}
