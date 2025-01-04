#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL2/SDL.h>
#include "config.h"
#include "bus.h"
#include "gameboy.h"

/*
// MEASURED IN 8x8 TILES, SO 80x80
#define TW 8
#define TH 8
#define W 10
#define H 10

struct pixel_sq {
    float value;
    // MAYBE SDL2 VALUES HERE OR WE USE AN SDL SQUARE STRUCT DIRECTLY
};

struct tile {
    struct pixel_sq pixels[TH][TW];
};
*/
void ui_init();
void ui_event_handler();
void ui_update();

#endif
