#ifndef HAZE_FORMAT_FORMAT_H_
#define HAZE_FORMAT_FORMAT_H_

#include <cstdint>
#include <string>
#include <haze.h>


constexpr uint32_t MAGIC4(char a, char b, char c, char d) {
    return (((uint32_t)(a)<<24)|((uint32_t)(b)<<16)|((uint32_t)(c)<<8)|(d));
}


// Format specifies a certain layout of binary data in a file. Closely related
// variants can share the same probe code, which can identify the format and suggest
// a player. For example, there are a single probe for different "mod" variants,
// including different four channel and multichannel trackers.

class Format {
    haze::FormatInfo info_;
public:
    Format(std::string const& id, std::string const& name): info_{id, name} {}
    virtual ~Format() {}

    haze::FormatInfo const& info() { return info_; }
    std::string& id() { return info_.id; }

    virtual bool probe(void *buf, uint32_t size, haze::ModuleInfo&) = 0;
};


#endif  // HAZE_FORMAT_FORMAT_H_
