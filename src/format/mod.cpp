#include "mod.h"
#include <haze.h>
#include "util/databuffer.h"

#define MAGIC_M_K_ MAGIC4('M', '.', 'K', '.')


bool ModFormat::probe(void *buf, uint32_t size, haze::ModuleInfo& mi)
{
    DataBuffer d(buf, size);

    auto magic = d.read32b(1080);

    switch (magic) {
    case MAGIC_M_K_:
        // TODO: check player acceptance
        mi.format_id = "m.k.";
        mi.title = d.read_string(0, 20);
        mi.description = "Amiga Protracker/Compatible";
        mi.creator = "unknown tracker";
        mi.channels = 4;
        mi.player = "pt21a";
        return true;
    }

    return false;
}

