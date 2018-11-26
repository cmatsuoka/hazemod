#include "format/format_registry.h"
#include "format/mod.h"
#include "format/fest.h"
#include "format/st.h"


FormatRegistry::FormatRegistry()
{
    this->insert(std::end(*this), {
        new ModFormat,
        new FestFormat,
        new StFormat,
    });
}

FormatRegistry::~FormatRegistry()
{
    for (auto item : *this) {
        delete item;
    }
}

