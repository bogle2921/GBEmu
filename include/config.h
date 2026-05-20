#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL_keycode.h>

// CORE TYPES
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;

// EMULATOR SETTINGS
#define EMU_TITLE "GBEmu"
// USED FOR WAYLAND APP-ID AND X11 WM_CLASS. SHOULD MATCH .desktop FILE BASENAME.
#define EMU_APP_ID "gbemu"
#define WINDOW_MULTI 5
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define EMU_WIDTH (SCREEN_WIDTH * WINDOW_MULTI)
#define EMU_HEIGHT (SCREEN_HEIGHT * WINDOW_MULTI)

// WINDOW BEHAVIOR
#define WINDOW_RESIZABLE    1   // 1 = USER CAN RESIZE; LETTERBOX MAINTAINS ASPECT
#define WINDOW_FULLSCREEN   0   // 1 = LAUNCH IN FULLSCREEN
#define WINDOW_VSYNC        1   // 1 = SDL VSYNC (TEARING OFF, COSTS A FEW MS)
#define KEY_TOGGLE_FULLSCREEN SDLK_F11

// LOGGING
#define LOG_TO_FILE     0   // 1 = MIRROR LOGS INTO logs/<component>.log
#define LOG_TO_STDOUT   0   // 1 = ECHO LOGS TO STDOUT
// !VERY EXPENSIVE, FOR TEST-LOG DIFFING ONLY!
#define LOG_VERBOSE_CPU 0

// DEBUG VIEWS
#define DEBUG_WINDOW    0   // 1 = OPEN SECOND SDL WINDOW WITH TILE ATLAS

// INPUT BINDINGS
// NOTE: EDIT BINDS HERE, USE SDLK_UNKNOWN TO LEAVE SLOT UNASSIGNED
#define KEY_GB_RIGHT_PRI  SDLK_RIGHT
#define KEY_GB_RIGHT_ALT  SDLK_d
#define KEY_GB_LEFT_PRI   SDLK_LEFT
#define KEY_GB_LEFT_ALT   SDLK_a
#define KEY_GB_UP_PRI     SDLK_UP
#define KEY_GB_UP_ALT     SDLK_w
#define KEY_GB_DOWN_PRI   SDLK_DOWN
#define KEY_GB_DOWN_ALT   SDLK_s
#define KEY_GB_A_PRI      SDLK_z
#define KEY_GB_A_ALT      SDLK_j
#define KEY_GB_B_PRI      SDLK_x
#define KEY_GB_B_ALT      SDLK_k
#define KEY_GB_SELECT_PRI SDLK_RSHIFT
#define KEY_GB_SELECT_ALT SDLK_BACKSPACE
#define KEY_GB_START_PRI  SDLK_RETURN
#define KEY_GB_START_ALT  SDLK_UNKNOWN
#define KEY_GB_QUIT       SDLK_ESCAPE

#endif
