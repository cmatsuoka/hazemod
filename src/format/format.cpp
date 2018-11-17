#include "format/format.h"
#include <iterator>
#include "format/mod.h"


FormatRegistry::FormatRegistry()
{
    this->insert(std::end(*this), {
        new ModFormat
    });
}

FormatRegistry::~FormatRegistry()
{
    for (auto item : *this) {
        delete item;
    }
}

