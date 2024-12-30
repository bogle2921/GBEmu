#ifndef CART_H
#define CART_H

#include "config.h"

struct rom_header{      // Content of header bytes: https://gbdev.io/pandocs/The_Cartridge_Header.html
    u8 entry[4];        // 0100-0103 — Entry point (After boot logo, jumps to $0100, then cartridge main)
    u8 logo[0x30];      // 0104-0133 — Nintendo logo (BMP Image displayed on boot. Must match specific hex dump otherwise wont run)
    char title[16];     // 0134-0143 — Title (Contains uppercase ASCII. If title is less than 16 char, remaining bytes should be $00 padded)
    /* UNCOMMENT FOR NEWER TARGET SYSTEM, SHOULD PROBABLY DO THIS PROGRAMATICALLY
    char man_code[4];   // 013F-0142 — Manufacturer code (Purpose Unknown, this only applies to newer models)
    u8 cgb_flag;        // 0143 — CGB flag (Color Mode, this only applies to newer models) */
    u16 license_new;    // 0144–0145 — New licensee code (Only applies if old license is exactly $33)
    u8 sgb;             // 0146 — SGB flag (Super Gameboy Cartridge Support)
    u8 type;            // 0147 — Cartridge type
    u8 size_rom;        // 0148 — ROM size code ( 32 KiB × (1 << <value>) )
    u8 size_ram;        // 0149 — RAM size code (Only applies if cart type includes "RAM" in name, else set to 0 / $00 )
    u8 dest;            // 014A — Destination code (Region Specifier)
    u8 license;         // 014B — Old licensee code (Only used for pre-SGB. If val is $33, use new licensee code instead)
    u8 version;         // 014C — Mask ROM version number (Usually $00)
    u8 checksum;        // 014D — Header checksum (8-bit checksum computed from the cartridge header bytes $0134–014C)
};

struct cartridge{
    char filename[1024];
    u32 rom_size;
    u8 *rom_data;
    u8 *ram_data;
    bool ram_enabled;
    struct rom_header* header;
};

bool load_cartridge(const char* cart);
void describe_cartridge(const struct cartridge* cart);
bool validate_header(const struct rom_header* header);
bool validate_checksum(const u8* rom_data, u8 expected_checksum);
bool validate_global_checksum(const u8* rom_data, u32 rom_size);
u8 read_cart(u16 addr);
void write_to_cart(u16 addr, u8 val);
u8 read_cart_ram(u16 addr);
void write_cart_ram(u16 addr, u8 val);
#endif