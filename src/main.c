#include "logger.h"
#include "SDL2/SDL.h"
#include "gameboy.h"
#include "cart.h"
#include "bus.h"
#include "config.h"

int main(int argc, char** argv) {
    
    // INIT LOGGER - CAREFUL SETTING INIT LEVEL TO TRACE...
    // LOG_TRACE WILL BASICALLY WRITE AS FAST AS YOU CAN WRITE TO DISK
    #ifdef DEBUG
    logger_init(LOG_TRACE);
    #else
    logger_init(LOG_DEBUG);
    #endif

    if (argc < 2) {
        LOG_WARN(LOG_MAIN, "Usage: %s [bootrom.bin] <game.gb>\n", argv[0]);
        return 1;
    }

    // INIT BUS FIRST
    init_bus();
    
    // INIT REST OF SYSTEMS
    gameboy_init();

    // LOAD BOOT ROM
    if (argc == 3 && !load_bootrom(argv[1])) {
        LOG_WARN(LOG_MAIN, "Bootrom missing / error: %s\n", argv[1]);
        return 1;
    }

    // LOAD CARTRIDGE
    if (!load_cartridge(argv[argc == 3 ? 2 : 1])) {
        LOG_ERROR(LOG_MAIN, "Failed to load cartridge: %s\n", argv[2]);
        return 1;
    }

    // RUN EMULATION
    run_gb();

    // CLEANUP
    logger_cleanup();
    
    return 0;
}