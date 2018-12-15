#include "format/xm.h"
#include <string>
#include <vector>
#include <haze.h>
#include "util/util.h"
#include "util/databuffer.h"


bool XmFormat::probe(void *buf, uint32_t size, haze::ModuleInfo& mi)
{
    const DataBuffer d(buf, size);

    if (d.read_string(0, 17) != "Extended Module: ") {
        return false;
    }

    const uint16_t ver = d.read16l(58);
    if (ver < 0x0102 || ver > 0x0104) {
        return false;
    }

    const int num_ord = d.read16l(64);
    const int num_chn = d.read16l(68);
    const int num_pat = d.read16l(70);
    const int num_ins = d.read16l(72);

    int offs = 80 + 256;

    if (ver >= 0x0104) {
        for (int i = 0; i < num_pat; i++) {
            const uint32_t size = d.read32l(offs);
            const uint16_t psize = d.read32l(offs + 7);
            offs += size + psize;
        }
    }

    // list instruments
    std::vector<std::string> ins_names;
    for (int i = 0; i < num_ins; i++) {
        const uint32_t size = d.read32l(offs);
        const auto name = d.read_string(offs + 4, 22);
        const int num_smp = d.read16l(offs + 27);

        offs += size;
        int total_sample_size = 0;

        for (int j = 0; j < num_smp; j++) {
            const uint32_t ssize = d.read32l(offs);
            offs += 40;
            total_sample_size += ssize;
        }
        offs += total_sample_size;
        ins_names.push_back(name);
    }

    mi.format_id = "xm";
    mi.title = d.read_string(17, 20);
    mi.description = "FastTracker II extended module";
    mi.creator = d.read_string(38, 20);
    mi.num_channels = num_chn;
    mi.length = num_ord;
    mi.num_instruments = num_ins;
    mi.instruments = ins_names;
    mi.player_id = "ft2";

    return true;
}
