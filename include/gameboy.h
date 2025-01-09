#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "config.h"
#include "cpu.h"
#include "graphics.h"
#include "timer.h"
#include "interrupt.h"
#include "dma.h"

// SYSTEM CONSTANTS
#define CPU_SPEED 4194304
#define CYCLES_PER_FRAME 70224
#define NS_PER_FRAME 16743706  // 1/59.7275 SEC IN NS
extern bool use_bootrom;

typedef struct {
    bool paused;
    u64 ticks;
    bool die;
    u32 cycles_this_frame;
} gameboy;

// CORE SYSTEM CONTROL
void gameboy_init();
void run_gb();
void gb_cycles(int cycles);
gameboy* get_gb();

#endif
