#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <string.h>
#include <SDL2/SDL.h>
#include "config.h"
#include "gameboy.h"


// SCREEN CONSTANTS  
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define VRAM_BANK_SIZE 0x2000
#define OAM_SIZE 0xA0
#define TILES_PER_LINE 32
#define TILE_SIZE 8
#define SPRITES_PER_LINE 10
#define MAX_SPRITES 40

// PPU MODES
#define MODE_HBLANK 0
#define MODE_VBLANK 1
#define MODE_OAM 2
#define MODE_DRAWING 3

// LCD CONTROL FLAGS
#define LCDC_BG_ENABLE     0x01
#define LCDC_OBJ_ENABLE    0x02
#define LCDC_OBJ_SIZE      0x04
#define LCDC_BG_MAP        0x08
#define LCDC_TILE_SELECT   0x10
#define LCDC_WINDOW_ENABLE 0x20
#define LCDC_WINDOW_MAP    0x40
#define LCDC_ENABLE        0x80

typedef struct {
    u8 y;
    u8 x; 
    u8 tile;
    u8 flags;
} sprite;

typedef struct {
    // PPU STATE
    bool lcd_enabled;
    u8 mode;
    u32 mode_clock;
    u8 line;
    bool lyc_interrupt;
    
    // LCD REGISTERS 
    u8 lcdc;
    u8 stat;
    u8 scy;
    u8 scx; 
    u8 ly;
    u8 lyc;
    u8 dma;
    u8 bgp;
    u8 obp0;
    u8 obp1;
    u8 wy;
    u8 wx;

    // MEMORY
    u8 vram[VRAM_BANK_SIZE];
    sprite oam[MAX_SPRITES];
    
    // FRAME BUFFER
    u32 frame_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];
    u32 bg_colors[4];
    u32 sprite_colors[2][4];
    
    // SDL
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    
    // DEBUG
    SDL_Window* debug_window;
    SDL_Renderer* debug_renderer; 
    SDL_Texture* debug_texture;
} graphics_system;

// INITIALIZATION
void graphics_init();
void graphics_cleanup();

// MAIN RENDERING PIPELINE
void graphics_tick();
void render_line();
void draw_frame();
void handle_events();

// MEMORY ACCESS
u8 vram_read(u16 addr);
void vram_write(u16 addr, u8 val);
u8 oam_read(u16 addr); 
void oam_write(u16 addr, u8 val);

// LCD ACCESS
u8 lcd_read(u16 addr);
void lcd_write(u16 addr, u8 val);

// DEBUG
void update_debug_window();
void dump_frame_buffer_sample();

#endif
