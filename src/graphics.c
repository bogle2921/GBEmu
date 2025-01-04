#include "graphics.h"

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_Surface *screen;

SDL_Window *debugWindow;
SDL_Renderer *debugRenderer;
SDL_Texture *debugTexture;
SDL_Surface *debugScreen;

static int tile_scale = 4;
static unsigned long tile_colors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

/*struct tile tiles[H][W] = {
    [0 ... H-1] = {
        [0 ... W-1] = {
            .pixels = {
                [0 ... TH-1] = {
                    [0 ... TW-1] = { .value = 0.0 }
                }
            }
        }
    }
};*/

void ui_init(){
    // START SDL - RETURN NEGATIVE INT ON FAILURE
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        exit(-1);
    } else {
        printf("SDL init succeeded\n");
    }

    // CREATE INITIAL WINDOW
    window = SDL_CreateWindow(
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
        exit(-1);;
    } else {
        printf("Window creation succeeded\n");
    }

    renderer = SDL_CreateRenderer(
        window, 
        -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    // EXIT CLEANLY ON FAILURE TO CREATE RENDERER
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(-1);
    } else {
        printf("Renderer creation succeeded\n");
    }

    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    //SDL_SetWindowPosition(window, x + EMU_WIDTH + 10, y);

    // DEBUG WINDOWS
    int debug_width = 16 * 8 * tile_scale;
    int debug_height = 32 * 8 *tile_scale;
    SDL_CreateWindowAndRenderer(debug_width, debug_height, 0, &debugWindow, &debugRenderer);
    debugScreen = SDL_CreateRGBSurface(0, (debug_width) + (16 * tile_scale), 
                                            (debug_height) + (64 * tile_scale), 32,
                                            0x00FF0000,
                                            0x0000FF00,
                                            0x000000FF,
                                            0xFF000000);
    
    debugTexture = SDL_CreateTexture(debugRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                    (debug_width) + (16 * tile_scale), (debug_height) + (64 * tile_scale));
}

void display_tile(SDL_Surface* surface, u16 addr, u16 tile, int x, int y){
    SDL_Rect rect;
    for(int tile_y = 0; tile_y < 16; tile_y += 2){
        u8 byte1 = read_from_bus(addr + (tile * 16) + tile_y);
        u8 byte2 = read_from_bus(addr + (tile * 16) + tile_y + 1);

        for(int bit = 7; bit >= 0; bit--){
            u8 high = !!(byte1 & (1 << bit)) << 1;
            u8 low = !!(byte2 & (1 << bit));

            u8 t_color = high | low;
            rect.x = x + ((7 - bit) * tile_scale);
            rect.y = y + (tile_y / 2 * tile_scale);
            rect.w = tile_scale;
            rect.h = tile_scale;

            SDL_FillRect(surface, &rect, tile_colors[t_color]);
        }
    }
}

void ui_event_handler(){
    SDL_Event e;
    while(SDL_PollEvent(&e) > 0){
        if(e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE){
            get_gb()->die = true;
        }
    }
}

void update_debug_win(){
    int xDraw = 0;
    int yDraw = 0;
    int tileNum = 0;

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = debugScreen->w;
    rect.h = debugScreen->h;
    SDL_FillRect(debugScreen, &rect, 0xFF111111);

    u16 addr = 0x8000;

    //384 tiles, 24 x 16
    for (int y=0; y<24; y++) {
        for (int x=0; x<16; x++) {
            display_tile(debugScreen, addr, tileNum, xDraw + (x * tile_scale), yDraw + (y * tile_scale));
            xDraw += (8 * tile_scale);
            tileNum++;
        }

        yDraw += (8 * tile_scale);
        xDraw = 0;
    }

	SDL_UpdateTexture(debugTexture, NULL, debugScreen->pixels, debugScreen->pitch);
	SDL_RenderClear(debugRenderer);
	SDL_RenderCopy(debugRenderer, debugTexture, NULL, NULL);
	SDL_RenderPresent(debugRenderer);
}
void ui_update(){
    update_debug_win();
}


