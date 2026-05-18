#include "logger.h"
#include "SDL2/SDL.h"
#include "gameboy.h"
#include "cart.h"
#include "bus.h"
#include "config.h"
#include <string.h>

int main(int argc, char** argv) {
    #ifdef DEBUG
    logger_init(LOG_TRACE);
    #else
    logger_init(LOG_DEBUG);
    #endif

    init_bus();

    bool bootrom_exists = false;
    const char* cart_path = NULL;

    if (argc == 2) {
        cart_path = argv[1];
    } else if (argc >= 3) {
        if (load_bootrom(argv[1])) {
            bootrom_exists = true;
            cart_path = argv[2];
        } else {
            LOG_WARN(LOG_MAIN, "Bootrom missing / error: %s - treating as cart\n", argv[1]);
            cart_path = argv[1];
        }
    }

    if (cart_path && !load_cartridge(cart_path)) {
        LOG_WARN(LOG_MAIN, "Failed to load cartridge '%s' - booting to splash\n", cart_path);
    }

    gameboy_init(bootrom_exists);
    run_gb();

    logger_cleanup();
    return 0;
}