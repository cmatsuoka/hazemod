#ifndef HAZE_FORMAT_FORMAT_H_
#define HAZE_FORMAT_FORMAT_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <haze.h>


constexpr uint32_t MAGIC4(char a, char b, char c, char d) {
    return (((uint32_t)(a)<<24)|((uint32_t)(b)<<16)|((uint32_t)(c)<<8)|(d));
}


class Format {
    std::string id_;
public:
    Format(std::string const& id): id_(id) {}
    virtual ~Format() {}
    std::string& id() { return id_; }
    virtual bool probe(void *buf, uint32_t size, haze::ModuleInfo&) = 0;
};


class FormatRegistry : public std::vector<Format *> {
public:
    FormatRegistry();
    ~FormatRegistry();
};


#endif  // HAZE_FORMAT_FORMAT_H_
