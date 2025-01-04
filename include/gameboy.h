#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "config.h"
#include "cpu.h"
#include "graphics.h"
#include "timer.h"
#include "ppu.h"

// typedef uint8_t u8;
// typedef uint16_t u16;
// typedef uint32_t u32;
// typedef uint64_t u64;
//typedef int8_t i8;

typedef struct {
    bool paused;
    u64 ticks;
    bool die;
} gameboy;

void gameboy_init();
gameboy* get_gb();
void run_gb();
void gameboy_init();
void gb_cycles(int cycles);


#endif