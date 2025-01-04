#ifndef LCD_H
#define LCD_H

#include "config.h"
#include "ppu.h"
#include "interrupt.h"
#include "dma.h"

typedef struct {
    // REGISTERS - in bus order
    u8 lcd_ctrl;
    u8 lcd_stat;
    u8 y_scroll;
    u8 x_scroll;
    u8 ly;
    u8 ly_comp;
    u8 dma;
    u8 bg_pallete;
    u8 sprite_pallete[2];
    u8 window_y;
    u8 window_x;

    // COLORS
    u32 bg_colors[4];
    u32 sprite_colors1[4];
    u32 sprite_colors2[4];
} lcd;

typedef enum {
    M_HBLANK,
    M_VBLANK,
    M_OAM,
    M_TX
} lcd_modes;

typedef enum {
    SRC_HBLANK = (1<<3),
    SRC_VBLANK = (1<<4),
    SRC_OAM = (1<<5),
    SRC_LYC = (1<<6),
} interrupt_src;

lcd *get_lcd();

/* HELPER MACROS */
// Get Register Bits
#define LCD_CTRL_BGW_EN (BIT_TST(get_lcd()->lcd_ctrl, 0))
#define LCD_CNTL_OBJ_EN (BIT_TST(get_lcd()->lcd_ctrl, 1))
#define LCD_CNTL_OBJ_HEIGHT (BIT_TST(get_lcd()->lcd_ctrl, 2) ? 16 : 8)
#define LCD_CNTL_BG_MAP (BIT_TST(get_lcd()->lcd_ctrl, 3) ? 0x9C00 : 0x9800)
#define LCD_CNTL_BGW_DATA (BIT_TST(get_lcd()->lcd_ctrl, 4) ? 0x8000 : 0x8800)
#define LCD_CTRL_WINDOW_EN (BIT_TST(get_lcd()->lcd_ctrl, 5))
#define LCD_CTRL_WINDOW_MAP (BIT_TST(get_lcd()->lcd_ctrl, 6) ? 0x9C00 : 0x9800)
#define LCD_CTRL_LCD_EN (BIT_TST(get_lcd()->lcd_ctrl, 7))

#define LCD_STAT_MODE ((lcd_modes)(get_lcd()->lcd_stat & 0b11))
#define LCD_SET_MODE (mode) {get_lcd()->lcd_stat &= ~0b11; get_lcd()->lcd_stat |= mode;}

#define LCD_GET_LYC (BIT_TST(get_lcd()->lcd_stat, 2))
#define LCD_SET_LYC_ON (BIT_ON(get_lcd()->lcd_stat, 2))
#define LCD_SET_LYC_OFF (BIT_OFF(get_lcd()->lcd_stat, 2))

#define LCD_STAT_INT_SRC(s) (get_lcd()->lcd_stat & s)

void lcd_init();
u8 lcd_read(u16 addr);
void lcd_write(u16 addr, u8 val);

#endif