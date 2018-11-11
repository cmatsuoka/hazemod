#ifndef HAZE_UTIL_OPTIONS_H_
#define HAZE_UTIL_OPTIONS_H_

#include <unordered_map>
#include <string>


class Options : public std::unordered_map<std::string, std::string> {

public:
    bool has_option(std::string const &k) {
        return find(k) != end();
    }

    int get(std::string const &k, int defl = 0) {
        if (!has_option(k)) {
            return defl;
        }
        return stoi((*this)[k], 0, 0);
    }
};


#endif  // HAZE_UTIL_OPTIONS_H_
