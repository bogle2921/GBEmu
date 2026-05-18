#ifndef APU_H
#define APU_H

#include "config.h"

void apu_init(void);
void apu_cleanup(void);
void apu_tick(void);     // CALLED ONCE PER M-CYCLE (4 T-CYCLES)
u8   apu_read(u16 addr);
void apu_write(u16 addr, u8 val);

#endif
