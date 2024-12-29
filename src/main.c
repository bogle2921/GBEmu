#include "SDL2/SDL.h"
#include "gameboy.h"
#include "cart.h"
#include "config.h"
#include "cpu.h"

// STATIC GAMEBOY INSTANCE
static struct gameboy gb = {0};

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

    // START CPU - AWAIT OPCODES
    cpu_init();
    printf("CPU initialized\n");

    // START SDL - RETURN NEGATIVE INT ON FAILURE
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return -1;
    } else {
        printf("SDL init succeeded\n");
    }

    // START GAMEBOY
    gameboy_init();
    printf("Gameboy initialized\n");

    // CREATE INITIAL WINDOW
    SDL_Window *window = SDL_CreateWindow(
        EMU_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        EMU_WIDTH,
        EMU_HEIGHT,

        // TODO: TRY OUT DIFFERENT SDL WIN OPTIONS, SDL_WINDOW_SHOWN ISNT ACTUALLY NEEDED
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
    );
    
    // EXIT CLEANLY ON FAILURE TO CREATE WINDOW
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    } else {
        printf("Window creation succeeded\n");
    }

 
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, 
        -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    // EXIT CLEANLY ON FAILURE TO CREATE RENDERER
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    } else {
        printf("Renderer creation succeeded\n");
    }


    while(gb.running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    goto out;
            }
        }

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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        // PLACEHOLDER RENDERING - BLACK SCREEN W/ WHITE SQUARE IN CORNER
        // TODO: TO HELP WITH DEBUG TILL WE GET TO RENDERING, DISPLAY CURRENT OPCODES + MISC DATA IN WINDOW TO SHOW SYNC
        SDL_Rect r;
        r.x = 10;
        r.y = 10;
        r.w = 40;
        r.h = 40;
        SDL_RenderFillRect(renderer, &r);

        // RENDER CURRENT FRAME
        SDL_RenderPresent(renderer);
    }

out:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

void gameboy_init() { // TODO: SHOULD POTENTIALLY ADD THIS TO CART.C OR NEW FILE
    gb.running = true;
    gb.paused = false;
    gb.ticks = 0;  
}
