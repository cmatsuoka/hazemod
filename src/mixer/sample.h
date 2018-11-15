#ifndef HAZE_MIXER_SAMPLE_H_
#define HAZE_MIXER_SAMPLE_H_

#include <cstdint>

// Sample flags
constexpr uint32_t Sample16Bits = 1 << 0;


class Sample {
    uint32_t flags_;
    void *data_;
    uint32_t size_;
    double rate_;
public:
    Sample(uint32_t flags = 0) :
        flags_(flags),
        data_(nullptr),
        size_(0),
        rate_(1.0) {}

    Sample(void *buf, uint32_t size, double rate = 1.0, uint32_t flags = 0) :
        flags_(flags),
        data_(buf),
        size_(size),
        rate_(rate) {}

    int16_t get(uint32_t pos) {
        return (pos >= size_) ? 0 : (flags_ & Sample16Bits) ?
            static_cast<int16_t *>(data_)[pos] :
            uint16_t(static_cast<uint8_t *>(data_)[pos]) - 128;  // convert to signed
    }

    uint32_t size() { return size_; }
    double rate() { return rate_; }
};

extern Sample empty_sample;


#endif  // HAZE_MIXER_SAMPLE_H_
