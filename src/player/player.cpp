#include "player/player.h"
#include "player/pt21a.h"
#include "player/nt11.h"
#include "player/st2.h"
#include "player/ust.h"


PlayerRegistry::PlayerRegistry()
{
    this->insert({
        { "pt2" , new Protracker2  },
        { "nt",   new Noisetracker },
        { "dst2", new DOC_Soundtracker_2 },
        { "ust",  new UltimateSoundtracker }
    });
}

PlayerRegistry::~PlayerRegistry()
{
    for (auto item : *this) {
        delete item.second;
    }
}
