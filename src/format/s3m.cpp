#include "format/s3m.h"
#include <string>
#include <vector>
#include <haze.h>
#include "util/util.h"
#include "util/databuffer.h"

#define MAGIC_SCRM      MAGIC4('S','C','R','M')
#define MAGIC_SCRI      MAGIC4('S','C','R','I')
#define MAGIC_SCRS      MAGIC4('S','C','R','S')


namespace {

class tracker_name
{
    uint16_t cwt_;
public:
    tracker_name(uint16_t cwt) : cwt_(cwt) {}
    std::string operator()(std::string const& name) { 
        return name + string_format(" %d.%02x", (cwt_ & 0x0f00) >> 8, cwt_ & 0xff);
    }
};

}  // namespace


bool S3mFormat::probe(void *buf, uint32_t size, haze::ModuleInfo& mi)
{
    const DataBuffer d(buf, size);

    if (size < 0x70 || d.read8(0x1c) != 0x1a || d.read8(0x1d) != 16 || d.read32b(0x2c) != MAGIC_SCRM) {
        return false;
    }

    uint16_t cwt = d.read16l(0x28);
    tracker_name version(cwt & 0xfff);

    switch (cwt >> 12) {
    case 1:
        mi.creator = version("Scream Tracker");
        break;
    case 2:
        mi.creator = version("Imago Orpheus");
        break;
    case 3:
        if (cwt == 0x3216) {
            mi.creator = "Impulse Tracker 2.14v3";
        } else if (cwt == 0x3217) {
            mi.creator = "Impulse Tracker 2.14v5";
        } else {
            mi.creator = version("Impulse Tracker");
        }
        break;
    case 5:
        mi.creator = version("OpenMPT %d.%02x");
        break;
    case 4:
        if (cwt != 0x4100) {
            mi.creator = version("Schism Tracker");
            break;
        }
        /* fall through */
    case 6:
        mi.creator = version("BeRoTracker");
        break;
    default:
        mi.creator = version("Unknown tracker");
    }

    int num_chn = 0;
    for (int i = 0; i < 32; i++) {
        uint8_t cset = d.read8(0x40 + i);
        if (cset == 0xff) {
            continue;
        }
        num_chn = i + 1;
    }

    int length = d.read16l(0x20);
    int num_ins = d.read16l(0x22);

    int i;
    std::vector<std::string> ins_names;
    for (i = 0; i < num_ins; i++) {
        //const auto name = d.read_string(48 + i * 32, 12);
        //ins_names.push_back(name);
    }


    mi.format_id = "s3m";
    mi.title = d.read_string(0, 28);
    mi.description = "Scream Tracker 3 module";
    mi.num_channels = num_chn;
    mi.length = length;
    mi.num_instruments = num_ins;
    mi.instruments = ins_names;
    mi.player_id = "st3";

    return true;
}
