#include "logger.h"
#include "gameboy.h"
#include "joypad.h"
#include "apu.h"
#include "cart.h"
#include <SDL2/SDL.h>
#include <string.h>

static gameboy GB;
static u64 PERF_FREQ;
static u64 TICKS_PER_FRAME;

gameboy* get_gb() {
    return &GB;
}

void gameboy_init(bool bootrom_enabled) {
    memset(&GB, 0, sizeof(GB));
    GB.bootrom_enabled = bootrom_enabled;

    // NOTE: DONT TRY TO INIT RAM HERE, IT SHOULD ALREADY BE DONE
    interrupt_init();  // INTERRUPTS FIRST
    timer_init();      // THEN OTHER SUBSYSTEMS
    cpu_init();
    joypad_init();
    graphics_init();   // ALSO INITS SDL (VIDEO + AUDIO)
    apu_init();        // NEEDS SDL_INIT_AUDIO

    PERF_FREQ = SDL_GetPerformanceFrequency();
    TICKS_PER_FRAME = PERF_FREQ / 60;  // TARGET 60 FPS, THIS IS MUCH BETTER
}

void gameboy_destroy() {
    LOG_DEBUG(LOG_MAIN, "CLEANING UP GAMEBOY MEMORY, GRAPHICS, ETC.\n");
    apu_cleanup();
    graphics_cleanup();
    cart_cleanup();    // PERSISTS BATTERY SAVE AND FREES ROM/RAM
}

void run_gb() {
    u64 last_tick = SDL_GetPerformanceCounter();
    u64 frame_time = PERF_FREQ / 60;

    while (!GB.die) {
        handle_events();

        bool emulate = !GB.paused && c.rom_data != NULL;

        if (emulate) {
            while (GB.cycles_this_frame < CYCLES_PER_FRAME) {
                cpu_step();
                u8 cycles = get_cpu_cycles();
                cycles += handle_interrupts();  // ADDS 20 T-CYCLES IF SERVICED

                GB.cycles_this_frame += cycles;

                // RUN PPU, TIMER, DMA, APU FOR EACH M-CYCLE (4 T-CYCLES)
                for (int t = 0; t < cycles; t += 4) {
                    timer_tick();
                    dma_tick();
                    graphics_tick();
                    apu_tick();
                }
            }
            GB.cycles_this_frame -= CYCLES_PER_FRAME;
        } else {
            draw_frame();
        }

        u64 now = SDL_GetPerformanceCounter();
        u64 elapsed = now - last_tick;
        if (elapsed < frame_time) {
            u64 delay = frame_time - elapsed;
            SDL_Delay((u32)(delay * 1000 / PERF_FREQ));
        }
        last_tick = SDL_GetPerformanceCounter();
    }

    gameboy_destroy();
}

void set_bootrom_enable(bool enable) {
    GB.bootrom_enabled = enable;
}

bool get_bootrom_enable() {
    return GB.bootrom_enabled;
}

// SAVE-STATE FILE PATH: <cart filename>.s<slot>
static void state_path(int slot, char* out, size_t out_sz) {
    snprintf(out, out_sz, "%s.s%d", c.filename, slot);
}

#define SAVE_STATE_MAGIC 0x53454247u  // "GBES" LE
#define SAVE_STATE_VERSION 2u

// FNV-1a 32-BIT HASH OF THE CARTRIDGE HEADER (0x100..0x14F). FINGERPRINTS
// THE ROM SO WE CAN REJECT SAVE STATES THAT BELONG TO A DIFFERENT CART.
static u32 rom_fingerprint(void) {
    if (!c.rom_data || c.rom_size < 0x150) return 0;
    u32 h = 0x811C9DC5u;
    for (int i = 0x100; i < 0x150; i++) {
        h ^= c.rom_data[i];
        h *= 0x01000193u;
    }
    return h;
}

bool gameboy_reset(void) {
    if (!c.rom_data) return false;

    // PERSIST BATTERY-BACKED RAM SO RESET DOESN'T LOSE PROGRESS
    if (c.has_battery) save_battery();

    GB.cycles_this_frame = 0;
    GB.paused = false;
    GB.ticks = 0;

    bus_reset();             // CLEARS WRAM / HRAM / IO, KEEPS BOOTROM
    if (c.has_battery) load_battery();  // RE-LOAD SAVE ON TOP OF CLEAR

    interrupt_init();
    timer_init();
    cpu_init();
    joypad_init();
    cart_reset();
    graphics_reset();
    apu_reset();
    return true;
}

bool gameboy_load_rom(const char* path) {
    if (!path || !path[0]) return false;

    // PERSIST CURRENT CART BEFORE WE TEAR IT DOWN
    if (c.has_battery) save_battery();
    cart_cleanup();

    if (!load_cartridge(path)) {
        LOG_ERROR(LOG_MAIN, "FAILED TO LOAD ROM: %s", path);
        return false;
    }

    // BOOTROM IS A ONE-SHOT THING. SKIP IT ON HOT-LOAD.
    GB.bootrom_enabled = false;

    if (!gameboy_reset()) return false;

    // REFRESH WINDOW TITLE WITH THE NEW ROM NAME
    extern void graphics_update_title_for_cart(void);
    graphics_update_title_for_cart();
    return true;
}

bool gameboy_save_state(int slot) {
    if (slot < 1 || slot > 4) return false;
    if (!c.rom_data || !c.filename[0]) return false;

    char path[1100];
    state_path(slot, path, sizeof(path));
    FILE* fp = fopen(path, "wb");
    if (!fp) {
        LOG_ERROR(LOG_MAIN, "SAVE STATE OPEN FAILED: %s", path);
        return false;
    }

    u32 magic = SAVE_STATE_MAGIC, version = SAVE_STATE_VERSION;
    u32 fp_hash = rom_fingerprint();
    fwrite(&magic,   sizeof(magic),   1, fp);
    fwrite(&version, sizeof(version), 1, fp);
    fwrite(&fp_hash, sizeof(fp_hash), 1, fp);

    cpu_save_state(fp);
    bus_save_state(fp);
    cart_save_state(fp);
    graphics_save_state(fp);
    apu_save_state(fp);
    timer_save_state(fp);
    interrupt_save_state(fp);
    dma_save_state(fp);
    joypad_save_state(fp);

    // ALSO PERSIST RELEVANT GB-LEVEL FLAGS
    fwrite(&GB.bootrom_enabled, sizeof(GB.bootrom_enabled), 1, fp);

    fclose(fp);
    LOG_INFO(LOG_MAIN, "SAVED STATE TO %s", path);
    return true;
}

bool gameboy_load_state(int slot) {
    if (slot < 1 || slot > 4) return false;
    if (!c.rom_data || !c.filename[0]) return false;

    char path[1100];
    state_path(slot, path, sizeof(path));
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        LOG_INFO(LOG_MAIN, "NO SAVE STATE AT %s", path);
        return false;
    }

    u32 magic = 0, version = 0, fp_hash = 0;
    fread(&magic,   sizeof(magic),   1, fp);
    fread(&version, sizeof(version), 1, fp);
    if (magic != SAVE_STATE_MAGIC || version != SAVE_STATE_VERSION) {
        LOG_ERROR(LOG_MAIN, "BAD SAVE STATE HEADER: %s", path);
        fclose(fp);
        return false;
    }
    fread(&fp_hash, sizeof(fp_hash), 1, fp);
    if (fp_hash != rom_fingerprint()) {
        LOG_ERROR(LOG_MAIN,
                  "SAVE STATE ROM FINGERPRINT MISMATCH (state=%08X, cart=%08X): %s",
                  fp_hash, rom_fingerprint(), path);
        fclose(fp);
        return false;
    }

    cpu_load_state(fp);
    bus_load_state(fp);
    cart_load_state(fp);
    graphics_load_state(fp);
    apu_load_state(fp);
    timer_load_state(fp);
    interrupt_load_state(fp);
    dma_load_state(fp);
    joypad_load_state(fp);

    fread(&GB.bootrom_enabled, sizeof(GB.bootrom_enabled), 1, fp);
    fclose(fp);

    // CYCLE COUNTER IS A SESSION THING, NOT IN THE FILE
    GB.cycles_this_frame = 0;
    LOG_INFO(LOG_MAIN, "LOADED STATE FROM %s", path);
    return true;
}