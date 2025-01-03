#include "dma.h"

struct dma DMA = {0};

void dma_start(u8 val){
    DMA.isActive = true;
    DMA.byte = 0;
    DMA.delay = 2;
    DMA.val = val;
}

void dma_tick(){
    if(!DMA.isActive){
        return;
    }

    if(DMA.delay){
        DMA.delay--;
        return;
    }

    oam_write(DMA.byte, read_from_bus((DMA.val * 0x100) + DMA.byte));
    DMA.byte++;
    DMA.isActive = DMA.byte < 0xA0;

    if(!DMA.isActive){
        printf("DMA Complete\n");
    }
}
bool dma_tx(){
    return DMA.isActive;
}