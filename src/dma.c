#include "logger.h"
#include "dma.h"
#include "bus.h"

static struct dma DMA = {0};

void dma_start(u8 val) {
    DMA.isActive = true;
    DMA.byte = 0;
    DMA.delay = 2;  // 2 CYCLE SETUP DELAY
    DMA.val = val;
}

void dma_tick() {
    if (!DMA.isActive) {
        return;
    }

    if (DMA.delay) {
        DMA.delay--;
        return;
    }

    // TRANSFER ONE BYTE FROM SOURCE TO OAM
    u16 source = (DMA.val << 8) + DMA.byte;
    write_to_bus(OAM_START + DMA.byte, read_from_bus(source));
    
    DMA.byte++;
    DMA.isActive = DMA.byte < 0xA0;  // 160 BYTES TOTAL
}

bool get_dma_active() {
    return DMA.isActive;  // TRUE IF TRANSFER IN PROGRESS
}
