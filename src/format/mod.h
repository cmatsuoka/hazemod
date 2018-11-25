#ifndef HAZE_FORMAT_MOD_H_
#define HAZE_FORMAT_MOD_H_

#include <haze.h>
#include "format/format.h"


class ModFormat : public Format {
public:
    ModFormat() : Format{"mod", "Amiga Protracker/Compatible"} {}
    bool probe(void *, uint32_t, haze::ModuleInfo&) override;
};

#endif  // HAZE_FORMAT_MOD_H_
