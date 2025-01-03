#include "io.h"
#include "config.h"

static char serial_data[2];

u8 ly = 0;

u8 io_read(u16 addr) {
    if (addr == 0xFF01) {
        return serial_data[0];
    }

    if (addr == 0xFF02) {
        return serial_data[1];
    }

    
    if (addr >= 0xFF04 && addr <= 0xFF07) {
        // return timer value
    }
    
    if (addr == 0xFF0F) {
        return get_int_flags();
    }
    

    if (addr == 0xFF44) {
        return ly++;
    }

    printf("UNSUPPORTED bus_read(%04X)\n", addr);
    return 0;
}

void io_write(u16 addr, u8 val) {
    if (addr == 0xFF01) {
        serial_data[0] = addr;
        return;
    }

    if (addr == 0xFF02) {
        serial_data[1] = addr;
        return;
    }

    
    if (addr >= 0xFF04 && addr <= 0xFF07) {
        // write timer value
    }
    

    if (addr == 0xFF0F) {
        set_int_flags(val);
    }
    

    if (addr == 0xFF46) {
        dma_start(val);
        printf("DMA START!\n");
    }

    printf("UNSUPPORTED bus_write(%04X)\n", addr);
}