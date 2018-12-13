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

    uint16_t ver = d.read16l(58);
    if (ver < 0x0102 || ver > 0x0104) {
        return false;
    }

    int num_chn = d.read16l(68);
    int num_ord = d.read16l(64);
    int num_ins = d.read16l(72);

    // list instruments
    std::vector<std::string> ins_names;
    for (int i = 0; i < num_ins; ++i) {
        const auto name = "";
        ins_names.push_back(name);
    }

    mi.format_id = "xm";
    mi.title = d.read_string(17, 20);
    mi.description = "Fasttracker II extended module";
    mi.creator = d.read_string(38, 20);
    mi.num_channels = num_chn;
    mi.length = num_ord;
    mi.num_instruments = num_ins;
    mi.instruments = ins_names;
    mi.player_id = "ft2";

    return true;
}
