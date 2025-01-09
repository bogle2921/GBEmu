#include "logger.h"
#include "gameboy.h"
#include <SDL2/SDL.h>
#include <string.h>

static gameboy GB;
static u64 PERF_FREQ;
static u64 TICKS_PER_FRAME;

bool use_bootrom = false;  // EXPOSED FOR BUS ACCESS

gameboy* get_gb() {
    return &GB;
}

void gameboy_init() {
    memset(&GB, 0, sizeof(GB));

    // NOTE: DONT TRY TO INIT RAM HERE, IT SHOULD ALREADY BE DONE
    interrupt_init();  // INTERRUPTS FIRST  
    timer_init();      // THEN OTHER SUBSYSTEMS      
    cpu_init();
    graphics_init();
    
    PERF_FREQ = SDL_GetPerformanceFrequency();
    TICKS_PER_FRAME = PERF_FREQ / 60;  // TARGET 60 FPS, THIS IS MUCH BETTER
}

void run_gb() {
    u64 last_tick = SDL_GetPerformanceCounter();
    u64 frame_time = PERF_FREQ / 60;

    while (!GB.die) {
        handle_events();

        // SYNC TO 4194304 CYCLES PER SECOND (59.7275 HZ)
        while (GB.cycles_this_frame < CYCLES_PER_FRAME) {
            if (!GB.paused) {
                handle_interrupts();
                cpu_step();
                
                u8 cycles = get_cpu_cycles();
                GB.cycles_this_frame += cycles;
                
                // RUN PPU, TIMER, DMA FOR EACH M-CYCLE (4 T-CYCLES)
                for (int t = 0; t < cycles; t += 4) {
                    timer_tick();
                    for (int i = 0; i < 4; i++) {
                        graphics_tick();
                    }
                    dma_tick();
                }
            }
        }

        // FRAME TIMING SYNC 
        u64 now = SDL_GetPerformanceCounter();
        u64 elapsed = now - last_tick;
        
        if (elapsed < frame_time) {
            u64 delay = frame_time - elapsed;
            SDL_Delay((u32)(delay * 1000 / PERF_FREQ));
        }
        
        last_tick = SDL_GetPerformanceCounter();
        GB.cycles_this_frame -= CYCLES_PER_FRAME;
    }
}
