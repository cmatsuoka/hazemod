#ifndef HAZE_FORMAT_XM_H_
#define HAZE_FORMAT_XM_H_

#include <haze.h>
#include "format/format.h"


class XmFormat : public Format {
public:
    XmFormat() : Format{"xm", "Fasttracker II"} {}
    bool probe(void *, uint32_t, haze::ModuleInfo&) override;
};

#endif  // HAZE_FORMAT_XM_H_
