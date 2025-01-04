#ifndef INTERRUPT_H
#define INTERRUPT_H


#include "config.h"
#include "stack.h"
#include "cpu.h"


typedef enum {
    INT_VBLANK = 1,
    INT_LCD = 2,
    INT_TIMER = 4,
    INT_SERIAL = 8,
    INT_JOYPAD = 16
} interrupts;

void interrupt_req(interrupts i);
void handle_interrupts(cpu* CPU);

#endif