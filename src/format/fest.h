#ifndef HAZE_FORMAT_FEST_H_
#define HAZE_FORMAT_FEST_H_

#include <haze.h>
#include "format/format.h"


class FestFormat : public Format {
public:
    FestFormat() : Format{"fest", "His Master's Noise"} {}
    bool probe(void *, uint32_t, haze::ModuleInfo&) override;
};

#endif  // HAZE_FORMAT_FEST_H_
