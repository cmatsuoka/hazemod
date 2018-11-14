#include "mod.h"
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <haze.h>
#include "util/databuffer.h"

#define MAGIC_M_K_ MAGIC4('M', '.', 'K', '.')


namespace {

// Known module variants

enum TrackerID {
    Unknown,
    Protracker,
    Noisetracker,
    Soundtracker,
    Screamtracker3,
    FastTracker,
    FastTracker2,
    Octalyser,
    TakeTracker,
    DigitalTracker,
    ModsGrave,
    FlexTrax,
    OpenMPT,
    Converted,
    ConvertedST,
    UnknownOrConverted,
    ProtrackerClone
};

// Known module magic IDs

struct Magic {
    uint32_t  magic;
    bool      flag;
    TrackerID id;
    uint8_t   ch;
};

Magic magic[13] = {
    { MAGIC4('M', '.', 'K', '.'), false, Protracker,     4 },
    { MAGIC4('M', '!', 'K', '!'), true,  Protracker,     4 },
    { MAGIC4('M', '&', 'K', '!'), true,  Noisetracker,   4 },
    { MAGIC4('N', '.', 'T', '.'), true,  Noisetracker,   4 },
    { MAGIC4('6', 'C', 'H', 'N'), false, FastTracker,    6 },
    { MAGIC4('8', 'C', 'H', 'N'), false, FastTracker,    8 },
    { MAGIC4('C', 'D', '6', '1'), true,  Octalyser,      6 },  // Atari STe/Falcon
    { MAGIC4('C', 'D', '8', '1'), true,  Octalyser,      8 },  // Atari STe/Falcon
    { MAGIC4('T', 'D', 'Z', '4'), true,  TakeTracker,    4 },  // see XModule SaveTracker.c
    { MAGIC4('F', 'A', '0', '4'), true,  DigitalTracker, 4 },  // Atari Falcon
    { MAGIC4('F', 'A', '0', '6'), true,  DigitalTracker, 6 },  // Atari Falcon
    { MAGIC4('F', 'A', '0', '8'), true,  DigitalTracker, 8 },  // Atari Falcon
    { MAGIC4('N', 'S', 'M', 'S'), true,  Unknown,        4 }   // in Kingdom.mod
};

// Default player based on module format variant

struct PlayerID {
    std::string name;
    std::string player_id;
};

std::unordered_map<TrackerID, PlayerID> tracker_map = {
    { Unknown,            { "unknown tracker",  "pt2" } },
    { Protracker,         { "Protracker",       "pt2" } },
    { Noisetracker,       { "Noisetracker",     "nt"  } },
    { Soundtracker,       { "Soundtracker",     "pt2" } },
    { Screamtracker3,     { "Scream Tracker 3", "st3" } },
    { FastTracker,        { "Fast Tracker",     "ft"  } },
    { FastTracker2,       { "Fast Tracker",     "ft2" } },
    { TakeTracker,        { "TakeTracker",      "ft2" } },
    { Octalyser,          { "Octalyser",        "ft"  } },
    { DigitalTracker,     { "Digital Tracker",  "pt2" } },
    { ModsGrave,          { "Mod's Grave",      "ft"  } },
    { FlexTrax,           { "FlexTrax",         "pt2" } },
    { OpenMPT,            { "OpenMPT",          "pt2" } },
    { Converted,          { "Converted",        "pt2" } },
    { ConvertedST,        { "Converted 15-ins", "nt"  } },
    { UnknownOrConverted, { "Unknown tracker",  "pt2" } },
    { ProtrackerClone,    { "Protracker clone", "pt2" } }
};

TrackerID id(DataBuffer const&);


}  // namespace



bool ModFormat::probe(void *buf, uint32_t size, haze::ModuleInfo& mi)
{
    const DataBuffer d(buf, size);

    uint32_t m = d.read32b(1080);

    // Check for known magic ID
    // FIXME: prevent false positives

    int num_ch = 0;
    for (auto entry : magic) {
        if (m == entry.magic) {
            num_ch = entry.ch;
            break;
        }
    }

    if (num_ch == 0) {
        if (isdigit(m >> 24) && ((m & 0x00ffffff) == MAGIC4(0, 'C', 'H', 'N'))) {
            num_ch = (m >> 24) - '0';
        } else if (isdigit(m >> 24) && isdigit((m & 0x00ff0000) >> 16) && ((m & 0x0000ffff) == MAGIC4(0, 0, 'C', 'H'))) {
            num_ch = ((m >> 24) - '0') * 10 + (((m & 0x00ff0000) >> 16) - '0');
        }
    }

    if (num_ch == 0) {
        return false;
    }

    // Magic found, identify tracker
    // FIXME: handle Mod's Grave WOW

    auto fmt = ::id(d);
    auto trk = tracker_map[fmt];

    std::string player = trk.player_id;;
    std::string format_id = "m.k.";

    if (num_ch != 4) {
        format_id = (num_ch <= 8) ? "xchn" : "xxch";
    }

    // TODO: check player acceptance
    mi.format_id = format_id;
    mi.title = d.read_string(0, 20);
    mi.description = "Amiga Protracker/Compatible";
    mi.creator = trk.name;
    mi.channels = num_ch;
    mi.player = player;

    return true;
}


namespace {

std::unordered_set<uint16_t> standard_notes = {
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
    428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
    214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113
}; 


// Try to identify the tracker used to create a module. This is a port of the fingerprint
// routine used in oxdz and libxmp.


bool has_large_instruments(DataBuffer const& d)
{
    for (int i = 0; i < 31; i++) {
        int ofs = 20 + 30 * i;
        int size = d.read16b(ofs + 22);

        if (size > 0x8000) {
            return true;
        }
    }
    return false;
}

// Check if has instruments with repeat length 0 
bool has_replen_0(DataBuffer const& d)
{
    for (int i = 0; i < 31; i++) {
        int ofs = 20 + 30 * i;
        int replen = d.read16b(ofs + 28);

        if (replen == 0) {
            return true;
        }
    }
    return false;
}

// Check if has instruments with size 0 and volume > 0
bool empty_ins_has_volume(DataBuffer const& d)
{
    for (int i = 0; i < 31; i++) {
        int ofs = 20 + 30 * i;
        int size = d.read16b(ofs + 22);
        int volume = d.read8(ofs + 25);

        if (size == 0 && volume > 0) {
            return true;
        }
    }
    return false;
}

bool size_1_and_volume_0(DataBuffer const& d)
{
    for (int i = 0; i < 31; i++) {
        int ofs = 20 + 30 * i;
        int size = d.read16b(ofs + 22);
        int volume = d.read8(ofs + 25);

        if (size == 1 && volume == 0) {
            return true;
        }
    }
    return false;
}

bool has_st_instruments(DataBuffer const& d)
{
    for (int i = 0; i < 31; i++) {
        int ofs = 20 + 30 * i;
        auto name = d.read_string(ofs, 22);

        if (name.length() < 6) {
            return false;
        }

        if (name[0] != 's' && name[0] != 'S') {
            return false;
        }
        if (name[1] != 't' && name[1] != 'T') {
            return false;
        }
        if (name[2] != '-' || name[5] != ':') {
            return false;
        }
        if (!isdigit(name[3]) || !isdigit(name[4])) {
            return false;
        }
    }
    return true;
}

bool has_ins_15_to_31(DataBuffer const& d)
{
    for (int i = 15; i < 31; i++) {
        int ofs = 20 + 30 * i;
        auto name = d.read_string(ofs, 22);
        int size = d.read16b(ofs + 22);

        if (name != "" || size > 0) {
            return true;
        }
    }
    return false;
}

TrackerID get_tracker_id(DataBuffer const& d, const int num_pat)
{
    TrackerID tracker_id = Unknown;
    bool detected = false;
    int chn = 0;

    uint32_t m = d.read32b(1080);

    for (auto entry : magic) {
        if (m == entry.magic) {
            tracker_id = entry.id;
            chn        = entry.ch;
            detected   = entry.flag;
            break;
        }
    }

    if (detected) {
        return tracker_id;
    }

    if (chn == 0) {
        int m0 = d.read8(1080);
        int m1 = d.read8(1081);
        int m2 = d.read8(1081);
        int m3 = d.read8(1081);

        if (isdigit(m0) && isdigit(m1) && m2 == 'C' && m3 == 'H') {
            chn = (m0 - '0') * 10 + m1 - '0';
        } else if (isdigit(m0) && m1 == 'C' && m2 == 'H' && m3 == 'N') {
            chn = m0 - '0';
        } else {
            return Unknown;
        }

        return chn & 1 ? TakeTracker : FastTracker2;
    }

    if (has_large_instruments(d)) {
        return OpenMPT;
    }

    bool replen_0 = has_replen_0(d);
    bool st_instruments = has_st_instruments(d);
    bool empty_ins_with_vol = empty_ins_has_volume(d);

    int restart = d.read8(951);

    if (restart == num_pat) {
        tracker_id = chn == 4 ? Soundtracker : Unknown;
    } else if (restart == 0x78) {
        // Not really sure, "MOD.Data City Remix" has Protracker effects and Noisetracker restart byte
        return chn == 4 ? Noisetracker : Unknown;
    } else if (restart < 0x7f) {
        tracker_id = (chn == 4 && !empty_ins_with_vol) ? Noisetracker : Unknown;
    } else if (restart == 0x7f) {
        if (chn == 4) {
            if (replen_0) {
                tracker_id = ProtrackerClone;
            }
        } else {
            tracker_id = Screamtracker3;
        }
        return tracker_id;
    } else if (restart > 0x7f) {
        return Unknown;
    }

    if (!replen_0) {  // All loops are size 2 or greater
        if (size_1_and_volume_0(d)) {
            return Converted;
        }

        if (!st_instruments) {
            for (int i = 0; i < 31; i++) {
                int ofs = 20 + 30 * i;
                int size = d.read16b(ofs + 22);
                int replen = d.read16b(ofs + 28);

                if (size != 0 || replen != 1) {
                    continue;
                }

                switch (chn) {
                case 4:
                    tracker_id = empty_ins_with_vol ? OpenMPT : Noisetracker;  // or Octalyser
                    break;
                case 6:
                    tracker_id = Octalyser;
                    break;
                case 8:
                    tracker_id = Octalyser;
                    break;
                default:
                    tracker_id = Unknown;
                };
                return tracker_id;
            }

            switch (chn) {
            case 4:
                tracker_id = Protracker;
                break;
            case 6:
                tracker_id = FastTracker;  // FastTracker 1.01?
                break;
            case 8:
                tracker_id = FastTracker;  // FastTracker 1.01?
                break;
            default:
                tracker_id = Unknown;
            }
        }
    } else {  // Has loops with size 0
        if (!has_ins_15_to_31(d)) {
            return ConvertedST;
        }
        if (st_instruments) {
            return UnknownOrConverted;
        } else if (chn == 6 || chn == 8) {
            return FastTracker;
        }
    }

    return tracker_id;
}

// Check if module uses only standard tracker octaves
bool has_standard_octaves(DataBuffer const& d, const int num_pat, const int num_ch)
{
    for (int i = 0; i < 64 * num_pat * num_ch * 4; i += 4) {
        uint32_t ev = d.read32b(1084 + i);
        int note = (ev & 0x0fff0000) >> 16;

        if (note != 0 && (note < 109 || note > 907)) {
            return false;
        }
    }
    return true;
}

// Check if module uses only standard tracker notes
bool has_standard_notes(DataBuffer const& d, const int num_pat, const int num_ch)
{
    for (int i = 0; i < 64 * num_pat * num_ch * 4; i += 4) {
        uint32_t ev = d.read32b(1084 + i);
        int note = (ev & 0x0fff0000) >> 16;

        if (note != 0 && standard_notes.find(note) == standard_notes.end()) {
            return false;
        }
    }
    return true;
}

// Check if module uses only NoiseTracker commands
bool has_only_nt_cmds(DataBuffer const& d, const int num_pat, const int num_ch)
{
    for (int i = 0; i < 64 * num_pat * num_ch * 4; i += 4) {
        uint32_t ev = d.read32b(1084 + i);
        uint8_t cmd = (ev & 0x0000ff00) >> 8;
        uint8_t cmdlo = ev & 0x000000ff;

        if ((cmd > 0x06 && cmd < 0x0a) || (cmd == 0x0e && cmdlo > 1)) {
            return false;
        }
    }
    return true;
}

TrackerID id(DataBuffer const& d)
{
    int num_pat = 0;
    for (int i = 0; i < 128; i++) {
        int pat = d.read8(952 + i);
        num_pat = std::max(num_pat, pat);
    }

    auto tracker_id = get_tracker_id(d, num_pat);
    uint8_t restart = d.read8(951);

    if (tracker_id == Noisetracker) {
        if (!has_only_nt_cmds(d, num_pat, 4) || !has_standard_notes(d, num_pat, 4)) {
            tracker_id = Unknown;
        }
    } else if (tracker_id == Soundtracker) {
        if (!has_standard_notes(d, num_pat, 4)) {
            tracker_id = Unknown;
        }
    } else if (tracker_id == Protracker) {
        if (restart != 0x7f || !has_standard_octaves(d, num_pat, 4)) {
            tracker_id = Unknown;
        }
    }

    // FIXME: are we really limited to 4 channels here?
    if (tracker_id == Unknown && restart == 0x7f && !has_standard_octaves(d, num_pat, 4)) {
        tracker_id = Screamtracker3;
    }

    return tracker_id;
}


}  // namespace
