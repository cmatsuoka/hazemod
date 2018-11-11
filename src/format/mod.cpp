#include "mod.h"
#include "util/databuffer.h"

#define MAGIC_M_K_ MAGIC4('M', '.', 'K', '.')


bool ModFormat::probe(void *buf, uint32_t size, haze::ProbeInfo& info)
{
    DataBuffer d(buf, size);

    auto magic = d.read32b(1080);

    switch (magic) {
    case MAGIC_M_K_:
        // TODO: check player acceptance
        info.id = "m.k.";
        info.title = d.read_string(0, 20);
        return true;
    }

    return false;
}
