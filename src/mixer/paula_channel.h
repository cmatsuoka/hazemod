#ifndef HAZE_MIXER_PAULA_CHANNEL_H_
#define HAZE_MIXER_PAULA_CHANNEL_H_

#include <cstdint>
#include <algorithm>
#include "util/databuffer.h"


class PaulaChannel {
    int rate;
    DataBuffer const& data;
    uint32_t pos;      // current sampling position
    uint32_t frac;     // 16-bit fractionary part of position
    uint32_t end;      // end position
    uint32_t audloc;   // AUDxLCH, AUDxLCL registers
    uint16_t audlen;   // AUDxLEN
    uint16_t audper;   // AUDxPER
    uint16_t audvol;   // AUDxVOL
    bool stopped;
    int8_t pan_;

public:
    PaulaChannel(DataBuffer const& d, const int sr) :
        rate(sr),
        data(d),
        pos(0),
        frac(0),
        end(0),
        audloc(0),
        audlen(0),
        audper(0),
        audvol(0),
        stopped(true) {}
    ~PaulaChannel() {}
    void set_start(const int32_t val)  { audloc = val; }
    void set_length(const uint16_t val) { audlen = val; }
    void set_period(const uint16_t val) { audper = val; }
    void set_volume(const uint16_t val) { audvol = std::min(val, uint16_t(64)); }
    void set_pan(const int8_t val) { pan_ = val; }
    int16_t volume() { return audvol; }
    int8_t pan() { return pan_; }

    void add_step() {
        const double step = 428.0 * 8287 / (rate * audper);
        const int32_t len = audlen * 2;
    
        frac += uint32_t((1 << 16) * step);
        pos += frac >> 16;
        frac &= (1 << 16) - 1;

        if (len > 2) {
            if (pos >= end) {
                end = audloc + audlen * 2;
                while (pos >= end) {
                    pos -= (end - audloc);
                }
            }
        }
    }

    int16_t get() {
        if (stopped || pos >= end) {
            return 0;
        }
        auto x = data.read8i(pos);
        add_step();
        return x * audvol;
    }

    void stop_dma() {
        stopped = true;
    }

    void start_dma() {
        pos = audloc;
        frac = 0;
        end = audloc + audlen * 2;
        stopped = false;
    }

    void reset() {
        stop_dma();
        pos = 0;
        frac = 0;
        end = 0;
        audloc = 0;
        audlen = 0;
        audper = 0;
        audvol = 0;
    }
};

#endif  // HAZE_MIXER_PAULA_CHANNEL_H_
