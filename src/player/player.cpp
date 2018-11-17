#include "player/player.h"
#include "player/pt21a.h"
#include "player/nt11.h"


PlayerRegistry::PlayerRegistry()
{
    this->insert({
        { "pt2", new Protracker2  },
        { "nt",  new Noisetracker },
    });
}

PlayerRegistry::~PlayerRegistry()
{
    for (auto item : *this) {
        delete item.second;
    }
}
