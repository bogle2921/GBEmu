#ifndef TIMER_H
#define TIMER_H

#include "config.h"
#include "interrupt.h"

// TAC REGISTER BITS
#define TAC_ENABLE      (1 << 2)    // TIMER ENABLE
#define TAC_FREQ_MASK   0x03        // FREQUENCY SELECT BITS

// TIMER FREQUENCIES (CPU CLOCK / N)
typedef enum {
    FREQ_4KHZ   = 0x00,    // 4096 HZ    (CPU/1024)
    FREQ_262KHZ = 0x01,    // 262144 HZ  (CPU/16)
    FREQ_65KHZ  = 0x02,    // 65536 HZ   (CPU/64)
    FREQ_16KHZ  = 0x03     // 16384 HZ   (CPU/256)
} timer_freq;

void timer_init(void);
void timer_tick(void);
u8 timer_read(u16 addr);
void timer_write(u16 addr, u8 val);

#endif
