#include "format/fest.h"
#include <haze.h>
#include "util/databuffer.h"

#define MAGIC_FEST MAGIC4('F', 'E', 'S', 'T')


bool FestFormat::probe(void *buf, uint32_t size, haze::ModuleInfo& mi)
{
    const DataBuffer d(buf, size);

    uint32_t m = d.read32b(1080);

    if (m != MAGIC_FEST) {
        return false;
    }

    mi.format_id = "fest";
    mi.title = d.read_string(0, 20);
    mi.description = "FEST module";
    mi.creator = "His Master's Noisetracker";
    mi.channels = 4;
    mi.length = d.read8(950);
    mi.player_id = "hmn";

    return true;
}
