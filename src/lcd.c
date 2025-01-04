#include "lcd.h"

static lcd LCD = {0};

// also in graphics.c -- maybe best to put in config.h?
//                                      white       light gray  dark gray   black
static unsigned long tile_colors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

lcd *get_lcd(){
    return &LCD;
}

void lcd_init(){
    LCD.lcd_ctrl = 0x91;
    LCD.x_scroll = 0;
    LCD.y_scroll = 0;
    LCD.ly = 0;
    LCD.ly_comp = 0;
    LCD.bg_pallete = 0xFC;
    LCD.sprite_pallete[0] = 0xFF;
    LCD.sprite_pallete[1] = 0xFF;
    LCD.window_y = 0;
    LCD.window_x = 0;

    for(int i = 0; i < 4; i++){
        LCD.bg_colors[i] = tile_colors[i];
        LCD.sprite_colors1[1] = tile_colors[i];
        LCD.sprite_colors2[1] = tile_colors[i];
    }
}

// this is why the struct was made in how the registers are set up
// better than if else?
u8 lcd_read(u16 addr){
    u8 offset = addr - 0xFF40;
    u8 *lcd_pointer = (u8 *)&LCD;
    return lcd_pointer[offset];
}

void update_pallete(u8 pallete, u8 data){
    // all bg and sprite palletes are the same
    u32 *colors = LCD.bg_colors;
    switch(pallete){
        case 1:
            colors = LCD.sprite_colors1;
            break;
        case 2:
            colors = LCD.sprite_colors2;
            break;
    }

    //white
    colors[0] = tile_colors[data & 0b11];
    // light gray
    colors[1] = tile_colors[(data >>2) & 0b11];
    // dark gray
    colors[2] = tile_colors[(data >>4) & 0b11];
    // black
    colors[3] = tile_colors[(data >>6) & 0b11];
}
void lcd_write(u16 addr, u8 val){
    u8 offset = addr - 0xFF40;
    u8 *lcd_pointer = (u8 *)&LCD;
    lcd_pointer[offset] = val;

    // check if we are at 0xFF46 - this is DMA
    if(offset == 6){
        dma_start(val);
    }

    //check if we are at a color pallete
    // for sprite (object) palletes white is ignored, transparent
    // so last 2 bits are taken off
    if(addr == 0xFF47){
        // update bg pallete
        update_pallete(0, val);
    } else if(addr == 0xFF47){
        // sprite pallete [0]
        update_pallete(1, val & 0b11111100);
    } else if(addr == 0xFF47){
        // sprite pallete [1]
        update_pallete(1, val & 0b11111100);
    }
}