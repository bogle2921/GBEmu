#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // unistd.h for linux

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;

#define EMU_TITLE "GBEmu"
#define WINDOW_MULTI 5
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define EMU_WIDTH SCREEN_WIDTH * WINDOW_MULTI
#define EMU_HEIGHT SCREEN_HEIGHT * WINDOW_MULTI

#define MAIN_RAM 8192
#define VIDEO_RAM 8192

#define BIT_TST(b, n) ((b & (1 << n)) ? 1 : 0)
#define BIT_ON(b, n) { b |= (1 << n);}
#define BIT_OFF(b, n) { b &= ~(1 << n);}

/*                                                          MEMORY MAP
                                        Start   End     Description                     Notes                                               */
#define ROM_START 0x0000 //             0000	3FFF	16 KiB ROM bank 00	            From cartridge, usually a fixed bank
#define ROM_BANK 0x4000 //              4000	7FFF	16 KiB ROM Bank 01–NN	        From cartridge, switchable bank via mapper (if any)
#define VRAM_START 0x8000 //            8000	9FFF	8 KiB Video RAM (VRAM)	        In CGB mode, switchable bank 0/1
#define RAM_START 0xA000 //             A000	BFFF	8 KiB External RAM	            From cartridge, switchable bank if any
#define WRAM_START 0xC000 //            C000	CFFF	4 KiB Work RAM (WRAM)
#define WRAM_START_SWITCHABLE 0xD000 // D000	DFFF	4 KiB Work RAM (WRAM)	        In CGB mode, switchable bank 1–7
#define ECHO_RAM 0xE000 //              E000	FDFF	Echo RAM (mirror of C000–DDFF)	Nintendo says use of this area is prohibited.
#define OAM_START 0xFE00 //             FE00	FE9F	Object attribute memory (OAM)
#define PROHIB_START 0xFEA0 //          FEA0	FEFF	Not Usable	                    Nintendo says use of this area is prohibited.
#define HMEM_START 0xFF00 //            FF00    FFFF    General rng for higher memory ops (start)
#define IO_REG 0xFF00 //                FF00	FF7F	I/O Registers
#define HRAM 0xFF80 //                  FF80	FFFE	High RAM (HRAM)	
#define IE_REG 0xFFFF //                FFFF	FFFF	Interrupt Enable register (IE)
#define HMEM_END 0xFFFF //              FF00    FFFF    General rng for higher memory ops (end), includes above sections up to HMEM_START.

#endif