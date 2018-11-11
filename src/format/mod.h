#ifndef HAZE_FORMAT_MOD_H_
#define HAZE_FORMAT_MOD_H_

#include "format/format.h"


class ModFormat : public Format {
public:
    ModFormat() : Format("mod") {}
    bool probe(void *, uint32_t, haze::ProbeInfo&) override;
};

#endif  // HAZE_FORMAT_MOD_H_
