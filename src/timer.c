#include "timer.h"

static timer_struct timer = {0};

timer_struct *get_timer(){
    return &timer;
}

void timer_init(){
    timer.div = 0xAC00;
}

void timer_tick(){
    u16 prev = timer.div;
    timer.div++;

    bool update = false;
    switch(timer.tac & (0b11)) {
        case 0b00:
            update = (prev & (1 << 9)) && (!(timer.div & (1 << 9)));
            break;
        case 0b01:
            update = (prev & (1 << 3)) && (!(timer.div & (1 << 3)));
            break;
        case 0b10:
            update = (prev & (1 << 5)) && (!(timer.div & (1 << 5)));
            break;
        case 0b11:
            update = (prev & (1 << 7)) && (!(timer.div & (1 << 7)));
            break;
    }

    if(update && timer.tac & (1<<2)){
        timer.tima++;
        if(timer.tima == 0xFF){
            timer.tima = timer.tma;
            //interrupt_req(INT_TIMER);
        }
    }
}
void timer_write(u16 addr, u8 val){
    switch(addr) {
        case 0xFF04:
            //DIV
            timer.div = 0;
            break;

        case 0xFF05:
            //TIMA
            timer.tima = val;
            break;

        case 0xFF06:
            //TMA
            timer.tma = val;
            break;

        case 0xFF07:
            //TAC
            timer.tac = val;
            break;
    }
}
u8 timer_read(u16 addr){
    switch(addr) {
        case 0xFF04:
            return timer.div >> 8;
        case 0xFF05:
            return timer.tima;
        case 0xFF06:
            return timer.tma;
        case 0xFF07:
            return timer.tac;
        default:
            printf("trying to read 0x%04X\n", addr);
            return 0xF;
    }
}
