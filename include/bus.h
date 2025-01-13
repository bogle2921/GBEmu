#ifndef BUS_H
#define BUS_H

#include "config.h"
#include "graphics.h"
#include "dma.h"
#include "cart.h"
#include "timer.h"
#include "interrupt.h"

// MEMORY MAP
#define ROM_START     0x0000  // 0000-7FFF: CART ROM (SWITCHABLE BANKS)
#define ROM_BANK      0x4000  // 4000: START OF SWITCHABLE BANK
#define VRAM_START    0x8000  // 8000-9FFF: VIDEO RAM
#define RAM_START     0xA000  // A000-BFFF: CART RAM
#define WRAM_START    0xC000  // C000-DFFF: WORK RAM  
#define ECHO_RAM      0xE000  // E000-FDFF: ECHO OF WRAM
#define OAM_START     0xFE00  // FE00-FE9F: SPRITE DATA
#define PROHIB_START  0xFEA0  // FEA0-FEFF: UNUSED
#define IO_START      0xFF00  // FF00-FF7F: IO REGS
#define HRAM          0xFF80  // FF80-FFFE: HIGH RAM  
#define IE_REG        0xFFFF  // FFFF: INTERRUPT ENABLE

// SPECIAL REGIONS
#define BOOT_ROM_SIZE 0x0100  // BOOTROM SIZE
#define RAM_BANK_SIZE 0x2000  // CART RAM BANK SIZE 
#define WAVE_RAM_END  0xFF3F  // END OF WAVE PATTERN RAM

// OFFSETS (FROM 0xFF00)
#define IO_JOYPAD    0x00  // JOYPAD INPUT
#define IO_SERIAL_SB 0x01  // SERIAL DATA
#define IO_SERIAL_SC 0x02  // SERIAL CONTROL
#define IO_DIV       0x04  // DIVIDER
#define IO_TIMA      0x05  // TIMER COUNTER
#define IO_TMA       0x06  // TIMER MODULO
#define IO_TAC       0x07  // TIMER CONTROL
#define IO_IF        0x0F  // INTERRUPT FLAGS
#define IO_NR10      0x10  // SWEEP
#define IO_NR11      0x11  // LENGTH/WAVE
#define IO_NR12      0x12  // ENVELOPE 
#define IO_NR13      0x13  // FREQ LO
#define IO_NR14      0x14  // FREQ HI
#define IO_NR21      0x16  // LENGTH/WAVE
#define IO_NR22      0x17  // ENVELOPE
#define IO_NR23      0x18  // FREQ LO
#define IO_NR24      0x19  // FREQ HI
#define IO_NR30      0x1A  // ON/OFF
#define IO_NR31      0x1B  // LENGTH
#define IO_NR32      0x1C  // LEVEL
#define IO_NR33      0x1D  // FREQ LO
#define IO_NR34      0x1E  // FREQ HI
#define IO_NR41      0x20  // LENGTH
#define IO_NR42      0x21  // ENVELOPE
#define IO_NR43      0x22  // POLY
#define IO_NR44      0x23  // COUNTER
#define IO_NR50      0x24  // MASTER VOL
#define IO_NR51      0x25  // ROUTING
#define IO_NR52      0x26  // POWER
#define IO_WAVE_RAM  0x30  // START OF WAVE PATTERN RAM (0xFF30-0xFF3F)
#define IO_LCDC      0x40  // LCD CONTROL
#define IO_STAT      0x41  // LCD STATUS
#define IO_SCY       0x42  // SCROLL Y
#define IO_SCX       0x43  // SCROLL X
#define IO_LY        0x44  // SCAN LINE
#define IO_LYC       0x45  // SCAN LINE CMP
#define IO_DMA       0x46  // DMA TRANSFER
#define IO_BGP       0x47  // BG PALETTE
#define IO_OBP0      0x48  // OBJ PAL 0
#define IO_OBP1      0x49  // OBJ PAL 1
#define IO_WY        0x4A  // WINDOW Y
#define IO_WX        0x4B  // WINDOW X
#define IO_BROM_DISABLE 0x50  // BOOTROM OFF
// TODO: GET COLOR WORKING
#define IO_VBK  0x4F   // VRAM BANK SELECT
#define IO_BCPS 0x68   // BG COLOR PALETTE SPEC
#define IO_BCPD 0x69   // BG COLOR PALETTE DATA  
#define IO_OCPS 0x6A   // OBJ COLOR PALETTE SPEC
#define IO_OCPD 0x6B   // OBJ COLOR PALETTE DATA

// FULL IO (0xFF00 + OFFSET)
#define BROM_UMAP       (IO_START + IO_BROM_DISABLE)
#define P1_REG          (IO_START + IO_JOYPAD)
#define SB_REG          (IO_START + IO_SERIAL_SB)
#define SC_REG          (IO_START + IO_SERIAL_SC)
#define DIV_REG         (IO_START + IO_DIV)
#define TIMA_REG        (IO_START + IO_TIMA)
#define TMA_REG         (IO_START + IO_TMA)
#define TAC_REG         (IO_START + IO_TAC)
#define IF_REG          (IO_START + IO_IF)
#define NR10_REG        (IO_START + IO_NR10)
#define NR11_REG        (IO_START + IO_NR11)
#define NR12_REG        (IO_START + IO_NR12)
#define NR13_REG        (IO_START + IO_NR13)
#define NR14_REG        (IO_START + IO_NR14)
#define NR21_REG        (IO_START + IO_NR21)
#define NR22_REG        (IO_START + IO_NR22)
#define NR23_REG        (IO_START + IO_NR23)
#define NR24_REG        (IO_START + IO_NR24)
#define NR30_REG        (IO_START + IO_NR30)
#define NR31_REG        (IO_START + IO_NR31)
#define NR32_REG        (IO_START + IO_NR32)
#define NR33_REG        (IO_START + IO_NR33)
#define NR34_REG        (IO_START + IO_NR34)
#define NR41_REG        (IO_START + IO_NR41)
#define NR42_REG        (IO_START + IO_NR42)
#define NR43_REG        (IO_START + IO_NR43)
#define NR44_REG        (IO_START + IO_NR44)
#define NR50_REG        (IO_START + IO_NR50)
#define NR51_REG        (IO_START + IO_NR51)
#define NR52_REG        (IO_START + IO_NR52)
#define WAVE_RAM_START  (IO_START + IO_WAVE_RAM)
#define LCDC_REG        (IO_START + IO_LCDC)
#define STAT_REG        (IO_START + IO_STAT)
#define SCY_REG         (IO_START + IO_SCY)
#define SCX_REG         (IO_START + IO_SCX)
#define LY_REG          (IO_START + IO_LY)
#define LYC_REG         (IO_START + IO_LYC)
#define DMA_REG         (IO_START + IO_DMA)
#define BGP_REG         (IO_START + IO_BGP)
#define OBP0_REG        (IO_START + IO_OBP0)
#define OBP1_REG        (IO_START + IO_OBP1)
#define WY_REG          (IO_START + IO_WY)
#define WX_REG          (IO_START + IO_WX)

void init_bus(void);
u8 read_from_bus(u16 addr);
void write_to_bus(u16 addr, u8 val);
u8* get_memory_ptr(u16 addr);
void debug_dump_bootrom(u8 num_bytes);
void load_bootrom_data(u8* data, u32 size);

#endif
