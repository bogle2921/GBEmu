#include "logger.h"
#include "SDL2/SDL.h"
#include "gameboy.h"
#include "cart.h"
#include "bus.h"
#include "config.h"

int main(int argc, char** argv) {
    // INIT LOGGER
    #ifdef DEBUG
    logger_init(LOG_TRACE);
    #else
    logger_init(LOG_DEBUG);
    #endif

    if (argc < 2) {
        LOG_WARN(LOG_MAIN, "Usage: %s [bootrom.bin] <game.gb>\n", argv[0]);
        return 1;
    }

    bool bootrom_exists = argc == 3;

    // INIT BUS FIRST
    init_bus();

    // LOAD BOOT ROM
    if (bootrom_exists && !load_bootrom(argv[1])) {
        LOG_WARN(LOG_MAIN, "Bootrom missing / error: %s\n", argv[1]);
        return 1;
    }

    // LOAD CARTRIDGE
    int rom_index = bootrom_exists ? 2 : 1;
    if (!load_cartridge(argv[rom_index])) {
        LOG_ERROR(LOG_MAIN, "Failed to load cartridge: %s\n", argv[rom_index]);
        return 1;
    }

    // INIT REST OF SYSTEMS
    gameboy_init(bootrom_exists);

    // RUN EMULATION
    run_gb();

    // CLEANUP
    logger_cleanup();
    
    return 0;
}