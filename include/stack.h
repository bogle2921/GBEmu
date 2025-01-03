#ifndef STACK_H
#define STACK_H

#include "config.h"
#include "cpu.h"
#include "bus.h"

void stack_push8(u8 val);
void stack_push16(u16 val);
u8 stack_pop8();
u16 stack_pop16();

#endif