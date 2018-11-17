#include "player/player.h"
#include "player/pt21a.h"
#include "player/nt11.h"

PlayerRegistry::PlayerRegistry()
{
    put(new PT21A);
    put(new NT11);
}


