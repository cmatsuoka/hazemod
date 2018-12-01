#ifndef HAZE_UTIL_DATABUFFER_H
#define HAZE_UTIL_DATABUFFER_H

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include "util/util.h"


class DataBuffer {
    uint8_t *data;
    uint32_t data_size;

public:
    DataBuffer() { data = nullptr; data_size = 0; }

    DataBuffer(void *buf, uint32_t size) {
        data = static_cast<uint8_t *>(buf);
        data_size = size;
    }

    uint8_t *ptr(const uint32_t ofs) const { return &data[ofs]; }

    uint32_t size() { return data_size; }

    uint8_t read8(const uint32_t ofs, const bool fail = true) const {
        return check_buffer_size(ofs + 1, fail) ? data[ofs] : 0;
    }    

    int8_t read8i(const uint32_t ofs, const bool fail = true) const {
        return check_buffer_size(ofs + 1, fail) ? static_cast<int8_t>(data[ofs]) : 0;
    }    

    uint16_t read16b(const uint32_t ofs, const bool fail = true) const {
        return check_buffer_size(ofs + 2, fail) ?
            (static_cast<uint16_t>(data[ofs]) << 8) |
             static_cast<uint16_t>(data[ofs + 1]) : 0;
    }

    uint16_t read16l(const uint32_t ofs, const bool fail = true) const {
        return check_buffer_size(ofs + 2, fail) ?
            (static_cast<uint16_t>(data[ofs + 1]) << 8) |
             static_cast<uint16_t>(data[ofs]) : 0;
    }

    uint32_t read32b(const uint32_t ofs, const bool fail = true) const {
        return check_buffer_size(ofs + 4, fail) ?
            (static_cast<uint32_t>(data[ofs]) << 24) |
            (static_cast<uint32_t>(data[ofs + 1]) << 16) |
            (static_cast<uint32_t>(data[ofs + 2]) << 8) |
             static_cast<uint32_t>(data[ofs + 3]) : 0;
    }

    uint32_t read32l(const uint32_t ofs, const bool fail = true) const {
        return check_buffer_size(ofs + 4, fail) ?
            (static_cast<uint32_t>(data[ofs + 3]) << 24) |
            (static_cast<uint32_t>(data[ofs + 2]) << 16) |
            (static_cast<uint32_t>(data[ofs + 1]) << 8) |
             static_cast<uint32_t>(data[ofs]) : 0;
    }

    std::string read_string(const uint32_t ofs, const uint32_t size, const bool fail = true) const {
        if (check_buffer_size(ofs + size, fail)) {
            char *buf = new char[size + 1];
            memcpy(buf, data + ofs, size);
            for (uint32_t i = 0; i < size && buf[i] != '\0'; i++) {
                if (!isprint(buf[i])) {
                    buf[i] = '.';
                }
            }
            buf[size] = '\0';
            auto ret = std::string(buf);
            delete [] buf;
            return ret;
        } else {
            return "";
        }
    }

    void write8(const uint32_t ofs, const uint8_t val) const {
        check_buffer_size(ofs + 1, true);
        data[ofs] = val;
    }    

    bool check_buffer_size(const uint32_t end, const bool fail) const {
        if (end > data_size || end <= 0) {
            if (fail) {
                throw std::runtime_error(string_format("short read (want %d bytes, have %d)", end, data_size));
            } else {
                return false;
            }
        }
        return true;
    }

};


#endif  // HAZE_UTIL_DATABUFFER_H
