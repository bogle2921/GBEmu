#ifndef IO_H
#define IO_H

#include "config.h"
#include "dma.h"
#include "cpu.h"
#include "timer.h"
#include "lcd.h"

u8 io_read(u16 addr);
void io_write(u16 addr, u8 val);

#endif