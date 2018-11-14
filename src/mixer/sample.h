#ifndef HAZE_MIXER_SAMPLE_H_
#define HAZE_MIXER_SAMPLE_H_

#include <cstdint>

// Sample flags
constexpr uint32_t Sample16Bits = 1 << 0;


class Sample {
    uint32_t flags;
    void *data;
public:
    int16_t get(uint32_t pos) {
        return (flags & Sample16Bits) ?
            static_cast<int16_t *>(data)[pos] :
            static_cast<uint8_t *>(data)[pos] << 8;
    }
};

extern Sample empty_sample;


#endif  // HAZE_MIXER_SAMPLE_H_
