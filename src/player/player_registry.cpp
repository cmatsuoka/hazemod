#include "player/player_registry.h"
#include "player/pt21a.h"
#include "player/nt11.h"
#include "player/dst2.h"
#include "player/ust.h"
#include "player/hmn.h"


PlayerRegistry::PlayerRegistry()
{
    this->insert({
        { "pt2" , new Protracker2  },
        { "nt",   new Noisetracker },
        { "dst2", new DOC_Soundtracker_2 },
        { "ust",  new UltimateSoundtracker },
        { "hmn",  new HisMastersNoise }
    });
}


PlayerRegistry::~PlayerRegistry()
{
    for (auto item : *this) {
        delete item.second;
    }
}
