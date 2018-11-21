#ifndef HAZE_MIXER_PAULA_CHANNEL_H_
#define HAZE_MIXER_PAULA_CHANNEL_H_

#include <cstdint>
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
    void set_length(const int16_t val) { audlen = val; }
    void set_period(const int16_t val) { audper = val; }
    void set_volume(const int16_t val) { audvol = val; }

    void add_step() {
        const double step = 428.0 * 8287 / (rate * audper);
        frac += uint32_t((1 << 16) * step);
        pos += frac >> 16;
        frac &= (1 << 16) - 1;
    }

    int16_t get(const int rate) {
        const int32_t len = audlen * 2;
    
        if (stopped || pos >= end) {
            return 0;
        }
    
        auto x = data.read8i(pos);
    
        add_step();
    
        if (len > 2) {
            if (pos >= end) {
                while (pos >= end) {
                    pos -= len;
                }
                end = audloc + audlen;
            }
        }
    
        return x;
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

};

#endif  // HAZE_MIXER_PAULA_CHANNEL_H_