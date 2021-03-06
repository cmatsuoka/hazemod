#include "player/player_registry.h"
#include "player/pt21a.h"
#include "player/nt11.h"
#include "player/dst2.h"
#include "player/ust.h"
#include "player/hmn.h"
#include "player/st2.h"
#include "player/st3.h"
#include "player/ft2.h"


PlayerRegistry::PlayerRegistry()
{
    this->insert({
        { "pt2" , new Protracker2  },
        { "nt",   new Noisetracker },
        { "dst2", new DOC_Soundtracker_2 },
        { "ust",  new UltimateSoundtracker },
        { "hmn",  new HisMastersNoise },
        { "st2",  new St2Player },
        { "st3",  new St3Player },
        { "ft2",  new Ft2Player }
    });
}


PlayerRegistry::~PlayerRegistry()
{
    for (auto item : *this) {
        delete item.second;
    }
}
