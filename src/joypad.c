#include "joypad.h"
#include "interrupt.h"

// P1 (FF00) LAYOUT:
//   BIT 7-6: UNUSED (READ AS 1)
//   BIT 5:   0 = ACTION BUTTONS SELECTED
//   BIT 4:   0 = DIRECTION BUTTONS SELECTED
//   BIT 3-0: BUTTON STATES OF THE SELECTED COLUMN (0 = PRESSED)
static u8 buttons;        // 1 BIT PER LOGICAL BUTTON, 1 = PRESSED
static u8 column_select;  // ONLY BITS 5 AND 4 ARE MEANINGFUL

void joypad_init(void) {
    buttons = 0;
    column_select = 0x30;  // BOTH COLUMNS DESELECTED
}

static u8 compute_lower_nibble(void) {
    u8 lower = 0x0F;  // NOTHING PRESSED

    // BIT 4 LOW = DIRECTIONS SELECTED
    if (!(column_select & 0x10)) {
        if (buttons & BTN_RIGHT) lower &= ~0x01;
        if (buttons & BTN_LEFT)  lower &= ~0x02;
        if (buttons & BTN_UP)    lower &= ~0x04;
        if (buttons & BTN_DOWN)  lower &= ~0x08;
    }
    // BIT 5 LOW = ACTIONS SELECTED
    if (!(column_select & 0x20)) {
        if (buttons & BTN_A)      lower &= ~0x01;
        if (buttons & BTN_B)      lower &= ~0x02;
        if (buttons & BTN_SELECT) lower &= ~0x04;
        if (buttons & BTN_START)  lower &= ~0x08;
    }
    return lower;
}

void joypad_press(gb_button b) {
    u8 before = compute_lower_nibble();
    buttons |= (u8)b;
    u8 after = compute_lower_nibble();
    // ANY 1->0 TRANSITION IN A SELECTED COLUMN RAISES INT_JOYPAD
    if ((before & ~after) != 0) {
        interrupt_req(INT_JOYPAD);
    }
}

void joypad_release(gb_button b) {
    buttons &= (u8)~b;
}

u8 joypad_read(void) {
    return 0xC0 | (column_select & 0x30) | compute_lower_nibble();
}

void joypad_write(u8 val) {
    // ONLY BITS 5 AND 4 ARE WRITABLE
    column_select = val & 0x30;
}
