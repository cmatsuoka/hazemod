#ifndef HAZE_FORMAT_ST_H_
#define HAZE_FORMAT_ST_H_

#include <haze.h>
#include "format/format.h"


class StFormat : public Format {
public:
    StFormat() : Format{"st", "Soundtracker"} {}
    bool probe(void *, uint32_t, haze::ModuleInfo&) override;
};

#endif  // HAZE_FORMAT_ST_H_
