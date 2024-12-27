#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "config.h"

struct gameboy {
    bool paused;
    bool running;
    u64 ticks;
};

#endif