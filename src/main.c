#include "SDL2/SDL.h"
#include "gameboy.h"
#include "cart.h"
#include "config.h"

int main(int argc, char** argv){
    if (argc < 2){
        printf("Usage: gb-emu <rom>\n");
        return -1;
    }

    const char* filename = argv[1];
    printf("Loading %s\n", filename);

    // VALIDATE + LOAD CART FROM ROMFILE
    if (!load_cartridge(filename)){
        printf("Failed to load: %s\n", filename);
        return -1;
    }

    gameboy_init();
    run_gb();

    /*while(gb.running){
        ui_event_handler();
        ui_update();

        // EXECUTE CPU INSTRUCTIONS FOR THIS FRAME
        // TODO: WE NEED PROPER TIMING!
        // -> https://gbdev.io/pandocs/Rendering.html?highlight=mhz
        if (!gb.paused) {
            // FROM DOCS: A “dot” = one 222 Hz (≅ 4.194 MHz) time unit
            // SO WE NEED TO RUN MULTIPLE CPU STEPS PER FRAME
            for (int i = 0; i < 69905; i++) {  // GUESTIMATE STEPS PER FRAME
                cpu_step();
            }
        }
    }*/
    return 0;
}


