#include "bus.h"

u8 read_from_bus(u16 addr){
    if(addr < VRAM_START){
        return read_cart(addr);
    }
    printf("TODO -- go further\n");
    exit(-1);
}

void write_to_bus(u16 addr, u8 val){
    if(addr < VRAM_START){
        // sometimes happens
        write_to_cart(addr, val);
    }
    printf("TODO -- go further\n");
    exit(-1);
}