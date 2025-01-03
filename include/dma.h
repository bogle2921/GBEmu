#ifndef DMA_H
#define DMA_H

#include "config.h"
#include "ppu.h"
#include "bus.h"

struct dma {
    bool isActive;
    u8 byte;
    u8 val;
    u8 delay;
};

void dma_start(u8 val);
void dma_tick();
bool dma_tx();

#endif