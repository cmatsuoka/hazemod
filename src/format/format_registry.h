#ifndef HAZE_FORMAT_FORMAT_REGISTRY_H_
#define HAZE_FORMAT_FORMAT_REGISTRY_H_

#include <vector>
#include "format.h"


class FormatRegistry : public std::vector<Format *> {
public:
    FormatRegistry();
    ~FormatRegistry();
};


#endif  // HAZE_FORMAT_FORMAT_REGISTRY_H_
