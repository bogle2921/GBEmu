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

typedef struct {
    bool paused;
    bool bootrom_enabled;
    u64 ticks;
    bool die;
    u32 cycles_this_frame;
} gameboy;

// CORE SYSTEM CONTROL
void gameboy_init();
void gameboy_destroy();
void run_gb();
gameboy* get_gb();
void set_bootrom_enable(bool enable);
bool get_bootrom_enable();

#endif
