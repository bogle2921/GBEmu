#ifndef CART_H
#define CART_H


#include "config.h"
#include "bus.h"
#include <time.h>
#include <string.h>

// CART TYPES
typedef enum {
   NO_MBC = 0x00,
   MBC1 = 0x01,
   MBC1_RAM = 0x02, 
   MBC1_RAM_BATTERY = 0x03,
   MBC2 = 0x05,
   MBC2_BATTERY = 0x06,
   MBC3_TIMER_BATTERY = 0x0F,
   MBC3_TIMER_RAM_BATTERY = 0x10,
   MBC3 = 0x11,
   MBC3_RAM = 0x12,
   MBC3_RAM_BATTERY = 0x13,
   MBC5 = 0x19,
   MBC5_RAM = 0x1A,
   MBC5_RAM_BATTERY = 0x1B
} cart_type;

// 0x0100-0x014F
struct rom_header {
   u8 entry[4];         // ENTRY POINT
   u8 logo[48];         // NINTENDO LOGO
   char title[16];      // GAME TITLE  
   u16 license_new;     // NEW LICENSE CODE
   u8 sgb;              // SGB FLAG
   u8 type;             // CARTRIDGE TYPE
   u8 size_rom;         // ROM SIZE
   u8 size_ram;         // RAM SIZE  
   u8 dest;             // DESTINATION CODE
   u8 license;          // OLD LICENSE CODE
   u8 version;          // ROM VERSION
   u8 checksum;         // HEADER CHECKSUM
   u16 global_checksum; // GLOBAL CHECKSUM
};

// CART STATE
struct cartridge {
   // FILE
   char filename[1024];
   char boot_filename[1024];
   u32 boot_rom_size;

   // MEMORY
   u8* rom_data;
   u8* ram_data;
   u32 rom_size;
   u32 ram_size;
   struct rom_header* header;

   // MBC STATE
   cart_type mbc_type;
   bool ram_enabled;
   bool has_battery;
   bool has_rtc;
   u8 current_rom_bank;
   u8 current_ram_bank; 
   u8 banking_mode;

   // RTC STATE (MBC3)
   u8 rtc_reg[5];    // S,M,H,DL,DH - NO IDEA WHAT THESE ARE USED FOR
   bool rtc_latched;
   time_t rtc_last;
};

extern struct cartridge c;

bool load_cartridge(const char* cart);
u8 read_cart(u16 addr);
void write_to_cart(u16 addr, u8 val);
u8 read_cart_ram(u16 addr);
void write_cart_ram(u16 addr, u8 val);

// VALIDATION
void describe_cartridge(const struct cartridge* cart);
bool validate_header(const struct rom_header* header);
bool validate_checksum(const u8* rom_data, u8 expected_checksum);
bool validate_global_checksum(const u8* rom_data, u32 rom_size);

// BOOTROM
bool load_bootrom(const char* bootrom);
void set_bootrom_enable(bool enable);
bool get_bootrom_enable(void);

// SAVE/RESTORE
bool save_battery(void);
bool load_battery(void);

#endif
