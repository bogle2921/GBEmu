#ifndef PPU_H
#define PPU_H

#include "config.h"

struct oam_data {
    u8 x;
    u8 y;
    u8 tile;
    u8 bg_priority : 1;
    u8 y_flip : 1;
    u8 x_flip : 1;
    u8 palette_num : 1;
    u8 bank : 1; // non gameboy color
    u8 gameboy_c_pallete_num : 3; // gameboy color only
};

struct ppu {
    struct oam_data oam_ram[40];
    u8 vram[0x2000];
};

void init_ppu();
void tick_ppu();

void oam_write(u16 addr, u8 val);
u8 oam_read(u16 addr);

void vram_write(u16 addr, u8 val);
u8 vram_read(u16 addr);
#endif