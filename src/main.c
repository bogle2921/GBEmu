#include "SDL2/SDL.h"
#include "gameboy.h"
#include "cart.h"

int main(int argc, char** argv){
    if(argc < 2){
        printf("Usage: gb-emu.exe <rom>\n");
        return -1;
    }

    const char* filename = argv[1];
    printf("Loading %s\n", filename);

    /*if(! load_cartridge(filename)){
        printf("Failed to load: %s\n", filename);
        return -1;
    }*/

    if(!load_cartridge(filename)){
        printf("Failed to load: %s\n", filename);
        return -1;
    }
    SDL_Init(SDL_INIT_EVERYTHING);

    struct gameboy gb;
    gb.running = true;
    gb.paused = false;
    gb.ticks = 0;   

    SDL_Window *window = SDL_CreateWindow(
        EMU_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        64,
        32,
        SDL_WINDOW_SHOWN);
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_TEXTUREACCESS_TARGET);

    while(gb.running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    goto out;
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0,0,0,0);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255,255,255,0);

        SDL_Rect r;
        r.x = 10;
        r.y = 10;
        r.w = 40;
        r.h = 40;
        SDL_RenderFillRect(renderer, &r); 
    }

out:
    SDL_DestroyWindow(window);
    return 0;
}