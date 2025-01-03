#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "config.h"
#include "cpu.h"
#include "graphics.h"
#include "timer.h"

typedef struct {
    bool paused;
    u64 ticks;
    bool die;
} gameboy;

void gameboy_init();
gameboy* get_gb();
void run_gb();
void gameboy_init();


#endif