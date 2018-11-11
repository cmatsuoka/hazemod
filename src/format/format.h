#ifndef HAZE_FORMAT_FORMAT_H_
#define HAZE_FORMAT_FORMAT_H_

#include <cstdint>
#include <string>

#define MAGIC4(a,b,c,d) \
    (((uint32_t)(a)<<24)|((uint32_t)(b)<<16)|((uint32_t)(c)<<8)|(d))


struct ProbeInfo {
    std::string id;
    std::string title;
};


class Format {
    std::string id_;
public:
    Format(std::string const& id): id_(id) {}
    std::string& id() { return id_; }
    virtual bool probe(void *buf, uint32_t size, ProbeInfo&) = 0;
};


#endif  // HAZE_FORMAT_FORMAT_H_
