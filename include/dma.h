#ifndef DMA_H
#define DMA_H

#include "config.h"
#include "graphics.h"

struct dma {
    bool isActive;
    u8 byte;
    u8 val;
    u8 delay;
};

void dma_start(u8 val);
void dma_tick();
bool get_dma_active();
void dma_save_state(FILE* fp);
void dma_load_state(FILE* fp);

#endif
