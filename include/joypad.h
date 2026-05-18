#ifndef JOYPAD_H
#define JOYPAD_H

#include "config.h"

// LOGICAL BUTTON BITS
typedef enum {
    BTN_RIGHT  = 0x01,
    BTN_LEFT   = 0x02,
    BTN_UP     = 0x04,
    BTN_DOWN   = 0x08,
    BTN_A      = 0x10,
    BTN_B      = 0x20,
    BTN_SELECT = 0x40,
    BTN_START  = 0x80,
} gb_button;

void joypad_init(void);
void joypad_press(gb_button b);
void joypad_release(gb_button b);

// TRUE IF ANY GB BUTTON IS CURRENTLY DOWN, REGARDLESS OF P14/P15 SELECTION.
// CPU STOP USES THIS TO WAKE - REAL HARDWARE WAKES STOP ON ANY KEY DOWN,
// NOT JUST WHEN A COLUMN IS SELECTED.
bool joypad_any_pressed(void);

// P1 REGISTER (FF00) I/O FROM THE BUS
u8   joypad_read(void);
void joypad_write(u8 val);

void joypad_save_state(FILE* fp);
void joypad_load_state(FILE* fp);

#endif
