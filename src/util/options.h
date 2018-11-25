#ifndef HAZE_UTIL_OPTIONS_H_
#define HAZE_UTIL_OPTIONS_H_

#include <initializer_list>
#include <unordered_map>
#include <string>


class Options : public std::unordered_map<std::string, std::string> {

public:
    Options()
    {
    }

    Options(std::initializer_list<value_type> list) :
        std::unordered_map<std::string, std::string>(list)
    {
    }

    bool has_option(std::string const& k) {
        return find(k) != end();
    }

    int get(std::string const& k, int defl) {
        if (!has_option(k)) {
            return defl;
        }
        return stoi((*this)[k], 0, 0);
    }

    std::string const& get(std::string const& k, std::string const& defl) {
        if (!has_option(k)) {
            return defl;
        }
        return (*this)[k];
    }
};


#endif  // HAZE_UTIL_OPTIONS_H_
