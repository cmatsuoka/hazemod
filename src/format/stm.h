#ifndef HAZE_FORMAT_STM_H_
#define HAZE_FORMAT_STM_H_

#include <haze.h>
#include "format/format.h"


class StmFormat : public Format {
public:
    StmFormat() : Format{"stm", "Scream Tracker 2"} {}
    bool probe(void *, uint32_t, haze::ModuleInfo&) override;
};

#endif  // HAZE_FORMAT_STM_H_
