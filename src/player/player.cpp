#include "player/player.h"
#include "player/pt21a.h"
#include "player/nt11.h"


PlayerRegistry::PlayerRegistry()
{
    put("pt2", new Protracker2);
    put("nt",  new Noisetracker);
}

PlayerRegistry::~PlayerRegistry()
{
    for (auto item : list()) {
        delete item;
    }
}
