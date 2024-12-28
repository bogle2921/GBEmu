#ifndef BUS_H
#define BUS_H

#include "config.h"

u8 read_from_bus(u16 addr);
void write_to_bus(u16 addr, u8 val);

#endif