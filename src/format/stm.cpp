#include "format/stm.h"
#include <string>
#include <vector>
#include <haze.h>
#include "util/util.h"
#include "util/databuffer.h"


bool StmFormat::probe(void *buf, uint32_t size, haze::ModuleInfo& mi)
{
    const DataBuffer d(buf, size);

    if (d.read8(28) != 0x1a || d.read8(29) != 2) {
        return false;
    }

    uint8_t maj = d.read8(30);
    uint8_t min = d.read8(31);
    uint8_t num_pat = d.read8(33);

    auto magic = d.read_string(20, 8);
    if (magic == "!Scream!") {
        mi.creator = "Scream Tracker";
    } else if (magic == "BMOD2STM" || magic == "WUZAMOD!") {
        mi.creator = magic;
    } else {
        return false;
    }

    mi.creator += string_format(" %d.%02d", maj, min);

    int i;
    std::vector<std::string> ins_names;
    for (i = 0; i < 31; i++) {
        const auto name = d.read_string(48 + i * 32, 12);
        ins_names.push_back(name);
    }

    for (i = 0; i < 128; i++) {
        if (d.read8(48 + 31 * 32 + i) >= num_pat) {
            break;
        }
    }
    mi.length = i;

    mi.format_id = "stm";
    mi.title = d.read_string(0, 20);
    mi.description = "Scream Tracker 2 module";
    mi.num_channels = 4;
    mi.num_instruments = 31;
    mi.instruments = ins_names;
    mi.player_id = "st2";

    return true;
}
