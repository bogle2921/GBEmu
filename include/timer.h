#ifndef TIMER_H
#define TIMER

#include "config.h"
#include "cpu.h"
#include "interrupt.h"

typedef struct {
    u16 div;
    u8 tima;
    u8 tma;
    u8 tac;
} timer_bus ;

void timer_init();
void timer_tick();
void timer_write(u16 addr, u8 val);
u8 timer_read(u16 addr);
#endif