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
   u16 current_rom_bank;   // u16 - MBC5 USES 9 BITS (UP TO 512 BANKS)
   u8 current_ram_bank;
   u8 banking_mode;

   // RTC STATE (MBC3 with timer)
   // rtc_reg[0..4]: SEC, MIN, HOUR, DAY_LO, DAY_HI
   //   DAY_HI bit 0 = day MSB, bit 6 = halt, bit 7 = day-counter carry.
   // rtc_reg IS THE LATCHED/VISIBLE VIEW; rtc_last IS THE HOST CLOCK AT THE
   // LAST LATCH SO WE CAN ADVANCE BY THE DELTA NEXT TIME.
   u8 rtc_reg[5];
   bool rtc_latched;
   time_t rtc_last;
   u8 rtc_latch_last;   // LAST VALUE WRITTEN TO THE 0x6000-0x7FFF LATCH REG

   // TODO: DYNAMICALLY SET RENDERING CLOCK MODES BASED ON COLOR MODE
   // CGB
   bool is_cgb;
   u8 mode3_length;
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

// SAVE/RESTORE
bool save_battery(void);
bool load_battery(void);

// FREES ROM/RAM AND PERSISTS BATTERY-BACKED SAVE
void cart_cleanup(void);

// RESET BANKING STATE; ROM/RAM ALLOCATIONS UNTOUCHED
void cart_reset(void);

// SAVE STATE: BANKING STATE + CART RAM CONTENTS (NOT ROM, NOT FILENAME)
void cart_save_state(FILE* fp);
void cart_load_state(FILE* fp);

// GETTERS/SETTERS
// get_cart_mode(void);

#endif
