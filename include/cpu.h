#ifndef CPU_H
#define CPU_H

#include "config.h"
#include "instructions.h"
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

// BIT MACROS FOR COMMON STUFF
#define BIT(n) (1 << (n))
#define BIT_SET(n, bit) ((n) |= BIT(bit))
#define BIT_CLEAR(n, bit) ((n) &= ~BIT(bit))
#define BIT_TEST(n, bit) ((n) & BIT(bit))

// FLAGS HELPERS
#define FLAG_Z BIT(7)
#define FLAG_N BIT(6)
#define FLAG_H BIT(5)
#define FLAG_C BIT(4)

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
    struct cpu_registers registers;
    u16 data_fetch;
    u16 memory_destination;
    u8 opcode;
    bool isHalted;
    bool isStepping;
    struct instruction* inst;
};

#endif