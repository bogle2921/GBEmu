#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define EMU_TITLE "GBEmu"
#define WINDOW_MULTI 5
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define EMU_WIDTH SCREEN_WIDTH * WINDOW_MULTI
#define EMU_HEIGHT SCREEN_HEIGHT * WINDOW_MULTI

#define MAIN_RAM 8192
#define VIDEO_RAM 8192

/*
MEMORY MAP
Start   End     Description                     Notes
0000	3FFF	16 KiB ROM bank 00	            From cartridge, usually a fixed bank
4000	7FFF	16 KiB ROM Bank 01–NN	        From cartridge, switchable bank via mapper (if any)
8000	9FFF	8 KiB Video RAM (VRAM)	        In CGB mode, switchable bank 0/1
A000	BFFF	8 KiB External RAM	            From cartridge, switchable bank if any
C000	CFFF	4 KiB Work RAM (WRAM)	
D000	DFFF	4 KiB Work RAM (WRAM)	        In CGB mode, switchable bank 1–7
E000	FDFF	Echo RAM (mirror of C000–DDFF)	Nintendo says use of this area is prohibited.
FE00	FE9F	Object attribute memory (OAM)	
FEA0	FEFF	Not Usable	                    Nintendo says use of this area is prohibited.
FF00	FF7F	I/O Registers	
FF80	FFFE	High RAM (HRAM)	
FFFF	FFFF	Interrupt Enable register (IE)	
*/
#define ROM_START 0x0000
#define ROM_BANK 0x4000
#define VRAM_START 0x8000
#define RAM_START 0xA000
#define WRAM_START 0xC000
#define WRAM_START_SWITCHABLE 0xD000 // yeah idk
#define ECHO_RAM 0xE000
#define OAM_START 0xFE00
#define PROHIB_START 0xFEA0
#define IO_REG 0xFF00
#define HRAM 0xFF80
#define IE_REG 0xFFFF

#endif