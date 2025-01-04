#include "ppu.h"

static struct ppu PPU = {0};

void init_ppu(){}
void ppu_tick(){}

void oam_write(u16 addr, u8 val){
    if(addr >= OAM_START){
        addr -= OAM_START;
    }
    u8* p = (u8*)PPU.oam_ram;
    p[addr] = val;
}

u8 oam_read(u16 addr){
    if(addr >= OAM_START){
        addr -= OAM_START;
    }
    u8* p = (u8*)PPU.oam_ram;
    return p[addr];
}

void vram_write(u16 addr, u8 val){
    // assumed to have offset at this time
    PPU.vram[addr - 0x8000] = val;
}
u8 vram_read(u16 addr){
    return PPU.vram[addr - 0x8000];
}