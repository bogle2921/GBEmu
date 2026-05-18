#ifndef APU_H
#define APU_H

#include "config.h"

void apu_init(void);
void apu_cleanup(void);
void apu_reset(void);    // CHANNEL STATE ONLY; KEEPS AUDIO DEVICE OPEN
void apu_tick(void);     // CALLED ONCE PER M-CYCLE (4 T-CYCLES)
u8   apu_read(u16 addr);
void apu_write(u16 addr, u8 val);

// USER CONTROLS
void  apu_set_master_volume(float v);   // 0.0..1.0
float apu_get_master_volume(void);
void  apu_set_channel_enabled(int ch, bool on);   // ch IN 0..3
bool  apu_get_channel_enabled(int ch);

void apu_save_state(FILE* fp);
void apu_load_state(FILE* fp);

#endif
