#include "logger.h"
#include "gameboy.h"
#include <SDL2/SDL.h>
#include <string.h>

static gameboy GB;
static Uint64 PERF_FREQ;
static Uint64 LAST_PERF_TICK;

bool use_bootrom = false;  // EXPOSED FOR BUS ACCESS

gameboy* get_gb() {
    return &GB;
}

void gameboy_init() {
    memset(&GB, 0, sizeof(GB));
    
    // REMOVE BUS INIT SINCE IT'S DONE IN MAIN
    interrupt_init();  // INTERRUPTS FIRST
    timer_init();      // THEN OTHER SUBSYSTEMS
    cpu_init();
    graphics_init();
    
    PERF_FREQ = SDL_GetPerformanceFrequency();
    LAST_PERF_TICK = SDL_GetPerformanceCounter();
}

static Uint64 get_elapsed_ns() {
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 diff = now - LAST_PERF_TICK;
    LAST_PERF_TICK = now;
    return (Uint64)((double)diff * 1000000000.0 / (double)PERF_FREQ);
}

void run_gb() {
    GB.cycles_this_frame = 0;

    while (!GB.die) {
        handle_events();

        while (GB.cycles_this_frame < CYCLES_PER_FRAME) {
            handle_interrupts();
            cpu_step();
            u8 c = get_cpu_cycles();
            GB.cycles_this_frame += c;
            gb_cycles(c);
            reset_cpu_cycles();
        }

        Uint64 ns = get_elapsed_ns();
        if (ns < NS_PER_FRAME) {
            Uint64 remain = NS_PER_FRAME - ns;
            Uint32 remain_ms = (Uint32)(remain / 1000000ULL);
            SDL_Delay(remain_ms);
        }

        GB.cycles_this_frame -= CYCLES_PER_FRAME;
    }

    graphics_cleanup();
}

void gb_cycles(int cycles) {
    for (int i = 0; i < cycles; i++) {
        timer_tick();
        for (int j = 0; j < 4; j++) {
            GB.ticks++;
            graphics_tick();
        }
        dma_tick();
    }
}
