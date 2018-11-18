#include "format/st.h"
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <haze.h>
#include "util/debug.h"
#include "util/databuffer.h"


namespace {

uint16_t note_table[37] = {
    856, 808, 762, 720, 678, 640, 604, 570,
    538, 508, 480, 453, 428, 404, 381, 360,
    339, 320, 302, 285, 269, 254, 240, 226,
    214, 202, 190, 180, 170, 160, 151, 143,
    135, 127, 120, 113, 0
};

bool test_name(uint8_t *b, uint32_t size)
{
    for (int i = 0; i < size; i++) {
        if (*b < 32 || *b > 0x7f) {
            return false;
        }
    }
    return true;
}

}  // namespace



bool StFormat::probe(void *buf, uint32_t size, haze::ModuleInfo& mi)
{
    const DataBuffer d(buf, size);

    // Ultimate Soundtracker support based on the module format description
    // written by Michael Schwendt
    bool ust = true;

    if (size < 600) {
        Debug("file too short");
        return false;
    }

    // check title
    if (!test_name(d.ptr(0), 20)) {
        Debug("invalid title");
        return false;
    }

    // check instruments
    int ofs = 20;
    int total_size = 0;
    for (int i = 0; i < 15; i++) {
        // Crepequs.mod has random values in first byte
        if (!test_name(d.ptr(ofs + 1), 21)) {
            Debug("sample %d invalid instrument name", i);
            return false;
        }

        const int size = d.read16b(ofs + 22);
        if (size > 0x8000) {
            Debug("sample %d invalid instrument size %d", i, size);
            return false;
        }

        if (d.read8(ofs + 24) != 0) {
            Debug("sample %d has finetune", i);
            return false;
        }

        if (d.read8(ofs + 25) > 0x40) {
            Debug("sample %d invalid volume", i);
            return false;
        }

        const int repeat = d.read16b(ofs + 26);
        if ((repeat >> 1) > size) {
            Debug("sample %d repeat > size", i);
            return false;
        }

        const int replen = d.read16b(ofs + 28);
        if (replen > 0x8000) {
            Debug("sample %d invalid replen", i);
            return false;
        }

        if (size > 0 && (repeat >> 1) == size) {
            Debug("sample %d repeat > size", i);
            return false;
        }

        if (size == 0 && repeat > 0) {
            Debug("sample {} invalid repeat", i);
            return false;
        }

        ofs += 30;
        total_size += size * 2;

        // UST: Maximum sample length is 9999 bytes decimal, but 1387
        // words hexadecimal. Longest samples on original sample disk
        // ST-01 were 9900 bytes.
        if (size > 0x1387 || repeat > 9999 || replen > 0x1387) {
            ust = false;
        }
    }

    if (total_size < 8) {
        Debug("invalid total sample size %d", total_size);
        return false;
    }

    // check length
    const int len = d.read8(470);
    if (len == 0 || len > 0x7f) {
        Debug("invalid length %d", len);
        return false;
    }

    // check tempo setting
    const int tempo = d.read8(471);
    if (tempo < 0x20) {
        Debug("invalid initial tempo %d", tempo);
        return false;
    }

    // UST: The byte at module offset 471 is BPM, not the song restart
    //      The default for UST modules is 0x78 = 120 BPM = 48 Hz.
    // if (tempo < 0x40) {    // should be 0x20
    //    ust = false
    // }

    // check orders
    uint8_t pat = 0;
    for (int i = 0; i < 128; i++) {
        uint8_t p = d.read8(472 + i);
        if (p > 0x7f) {
            Debug("invalid pattern number %d in orders", p);
            return false;
        }
        pat = std::max(pat, p);
    }
    pat++;

    // check patterns
    uint32_t cmd_used = 0;
    for (int i = 0; i < pat * 64 * 4 * 4; i += 4) {
        const int ofs = 600 + i;
        const int note = d.read16b(ofs);
        if (note & 0xf000) {
            Debug("invalid event sample");
            return false;
        }
        // check if note in table
        if (note != 0 && std::find(std::begin(note_table), std::end(note_table), note)) {
            Debug("invalid note %d", note);
            return false;
        }
        // check invalid commands
        const uint8_t cmd = d.read8(ofs + 2) & 0x0f;
        const uint8_t cmdlo = d.read8(ofs + 3);
        if (cmd) {
            cmd_used |= 1 << cmd;
        } else if (cmdlo) {
            cmd_used |= 1;
        }

        // UST: Only effects 1 (arpeggio) and 2 (pitchbend) are available
        if (cmd == 1 && cmdlo == 0) {  // unlikely arpeggio
            ust = false;
        }
        if (cmd == 2 && (cmdlo & 0xf0) && (cmdlo & 0x0f)) {  // bend up and down at same time
            ust = false;
        }
    }

    if (cmd_used & 0xfff9) {
        ust = false;
    }

    mi.format_id = ust ? "ust" : "st";
    mi.title = d.read_string(0, 20);
    mi.description = "15 instrument module";
    mi.creator = ust ? "Ultimate Soundtracker" : "Soundtracker";
    mi.channels = 4;
    mi.player_id = ust ? "ust" : "dst2";

    return true;
}
