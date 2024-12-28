#ifndef CPU_H
#define CPU_H

#include "config.h"
/*
REGISTERS
16-bit	Hi	Lo	Name/Function
AF	    A	-	Accumulator & Flags
BC	    B	C	BC
DE	    D	E	DE
HL	    H	L	HL
SP	    -	-	Stack Pointer
PC	    -	-	Program Counter/Pointer

FLAGS Register - LSB of AF
Bit	Name	Explanation
7	z	    Zero flag
6	n	    Subtraction flag (BCD)
5	h	    Half Carry flag (BCD)
4	c	    Carry flag
*/

struct cpu_registers {
    u8 a;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 f;
    u8 h;
    u8 l;
    u16 sp;
    u16 pc;
};

struct cpu {
    cpu_registers registers;
    u16 data_fetch;
    u16 memory_destination;
    u8 opcode;
    bool isHalted;
    bool isStepping;
};

#endif