#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "config.h"
#include "cpu.h"  // NEED CPU FOR REGISTER ACCESS

typedef enum {
    INT_VBLANK = 1,
    INT_LCD = 2,
    INT_TIMER = 4,
    INT_SERIAL = 8,
    INT_JOYPAD = 16
} interrupts;

void interrupt_init();
void interrupt_req(interrupts i);
void handle_interrupts();
u8 get_interrupt_flags();
void set_interrupt_flags(u8 flags);
u8 get_interrupt_enable();
void set_interrupt_enable(u8 enable);

#endif
