#include "format/format.h"
#include "format/mod.h"

FormatRegistry::FormatRegistry()
{
    put("mod", new ModFormat);
}

FormatRegistry::~FormatRegistry()
{
    for (auto item : list()) {
        delete item;
    }
}

