#ifndef CART_H
#define CART_H

#include "config.h"

struct rom_header{
    u8 entry[4];
    u8 logo[0x30];
    char title[16];
    u16 license_new;
    u8 gb_flag;
    u8 type;
    u8 size_rom;
    u8 size_ram;
    u8 dest;
    u8 license;
    u8 version;
    u8 checksum;
    u16 g_checksum;
};

struct cartridge{
    char filename[1024];
    u32 rom_size;
    u8* rom_data;
    struct rom_header* header;
};

bool load_cartridge(const char* cart);
#endif