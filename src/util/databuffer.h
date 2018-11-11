#ifndef HAZE_UTIL_DATABUFFER_H
#define HAZE_UTIL_DATABUFFER_H

#include <cstdint>
#include <stdexcept>
#include "util/util.h"


class DataBuffer {
    uint8_t *data;
    uint32_t data_size;

public:
    //DataBuffer() { data = nullptr; data_size = 0; }

    DataBuffer(void *buf, uint32_t size) {
        data = static_cast<uint8_t *>(buf);
        data_size = size;
    }

    uint8_t *ptr(uint32_t ofs) { return &data[ofs]; }

    uint32_t size() { return data_size; }

    uint8_t read8(uint32_t ofs) {
        check_buffer_size(ofs + 1);
        return data[ofs];
    }    

    int8_t read8i(uint32_t ofs) {
        check_buffer_size(ofs + 1);
        return static_cast<int8_t>(data[ofs]);
    }    

    uint16_t read16b(uint32_t ofs) {
        check_buffer_size(ofs + 2);
        return (static_cast<uint16_t>(data[ofs]) << 8) |
               static_cast<uint16_t>(data[ofs + 1]);
    }

    uint16_t read16l(uint32_t ofs) {
        check_buffer_size(ofs + 2);
        return (static_cast<uint16_t>(data[ofs + 1]) << 8) |
               static_cast<uint16_t>(data[ofs]);
    }

    uint32_t read32b(uint32_t ofs) {
        check_buffer_size(ofs + 2);
        return (static_cast<uint32_t>(data[ofs]) << 24) |
               (static_cast<uint32_t>(data[ofs + 1]) << 16) |
               (static_cast<uint32_t>(data[ofs + 2]) << 8) |
               static_cast<uint32_t>(data[ofs + 3]);
    }

    uint32_t read32l(uint32_t ofs) {
        check_buffer_size(ofs + 2);
        return (static_cast<uint32_t>(data[ofs + 3]) << 24) |
               (static_cast<uint32_t>(data[ofs + 2]) << 16) |
               (static_cast<uint32_t>(data[ofs + 1]) << 8) |
               static_cast<uint32_t>(data[ofs]);
    }

    void check_buffer_size(uint32_t end) {
        if (end > data_size || end <= 0) {
            throw std::runtime_error(string_format("short read (want %d bytes, have %d)", end, data_size));
        }
    }

};


#endif  // HAZE_UTIL_DATABUFFER_H