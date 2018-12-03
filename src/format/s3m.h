#ifndef HAZE_FORMAT_S3M_H_
#define HAZE_FORMAT_S3M_H_

#include <haze.h>
#include "format/format.h"


class S3mFormat : public Format {
public:
    S3mFormat() : Format{"s3m", "Scream Tracker 3"} {}
    bool probe(void *, uint32_t, haze::ModuleInfo&) override;
};

#endif  // HAZE_FORMAT_S3M_H_
