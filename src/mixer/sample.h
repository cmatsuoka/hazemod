#ifndef HAZE_MIXER_SAMPLE_H_
#define HAZE_MIXER_SAMPLE_H_

#include <cstdint>

// Sample flags
constexpr uint32_t Sample16Bits = 1 << 0;


class Sample {
    uint32_t flags_;
    void *data_;
    uint32_t size_;
public:
    Sample() : data_(nullptr), size_(0) {}

    Sample(void *buf, uint32_t size) : data_(buf), size_(size) {}

    int16_t get(uint32_t pos) {
        return (flags_ & Sample16Bits) ?
            static_cast<int16_t *>(data_)[pos] :
            static_cast<uint8_t *>(data_)[pos] << 8;
    }

    uint32_t size() { return size_; }
};

extern Sample empty_sample;


#endif  // HAZE_MIXER_SAMPLE_H_
