#include "format/format_registry.h"
#include "format/mod.h"
#include "format/xm.h"
#include "format/s3m.h"
#include "format/stm.h"
#include "format/fest.h"
#include "format/st.h"


FormatRegistry::FormatRegistry()
{
    this->insert(std::end(*this), {
        new ModFormat,
        new XmFormat,
        new S3mFormat,
        new StmFormat,
        new FestFormat,
        new StFormat
    });
}

FormatRegistry::~FormatRegistry()
{
    for (auto item : *this) {
        delete item;
    }
}

