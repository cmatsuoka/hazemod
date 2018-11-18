#include "player/player.h"
#include "player/pt21a.h"
#include "player/nt11.h"
#include "player/ust.h"


PlayerRegistry::PlayerRegistry()
{
    this->insert({
        { "pt2", new Protracker2  },
        { "nt",  new Noisetracker },
        { "ust", new UltimateSoundtracker }
    });
}

PlayerRegistry::~PlayerRegistry()
{
    for (auto item : *this) {
        delete item.second;
    }
}
