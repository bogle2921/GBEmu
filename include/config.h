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

#endif