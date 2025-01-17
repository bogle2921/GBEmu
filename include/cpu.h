#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "config.h"
#include "bus.h"

// REGISTERS
typedef struct {
    u8 a, b, c, d, e, h, l, f;
    u16 pc, sp;
} registers;

// CPU STATE
typedef struct {
    registers reg;
    bool halted;
    bool halt_bug;
    bool stopped;
    bool stepping;
    bool ime;
    bool ime_scheduled;
    u8 opcode;
    u32 cycles;
} cpu;

// FLAGS
#define FLAG_Z (1 << 7)  // ZERO
#define FLAG_N (1 << 6)  // SUBTRACT
#define FLAG_H (1 << 5)  // HALF CARRY
#define FLAG_C (1 << 4)  // CARRY

// HELPERS
#define SET_FLAG_Z(x) CPU.reg.f = (CPU.reg.f & ~FLAG_Z) | ((x) ? FLAG_Z : 0)
#define SET_FLAG_N(x) CPU.reg.f = (CPU.reg.f & ~FLAG_N) | ((x) ? FLAG_N : 0)
#define SET_FLAG_H(x) CPU.reg.f = (CPU.reg.f & ~FLAG_H) | ((x) ? FLAG_H : 0)
#define SET_FLAG_C(x) CPU.reg.f = (CPU.reg.f & ~FLAG_C) | ((x) ? FLAG_C : 0)
#define GET_FLAG_Z  ((CPU.reg.f >> 7) & 0x01)
#define GET_FLAG_N  ((CPU.reg.f >> 6) & 0x01)
#define GET_FLAG_H  ((CPU.reg.f >> 5) & 0x01)
#define GET_FLAG_C  ((CPU.reg.f >> 4) & 0x01)
#define GET_HIGH_BYTE(sl) ((sl >> 8) & 0xFF)
#define GET_LOW_BYTE(fl) (fl & 0xFF)
#define MAKE_WORD(fl,sl) ((fl << 8) | sl)

// SETUP AND CYCLE CPU
void cpu_init(void);
void cpu_step(void);
u8 get_cpu_cycles(void);
void reset_cpu_cycles(void);

// CPU STATE ACCESS
registers* get_registers();
bool get_ime(void);
void set_ime(bool val);
bool is_cpu_halted(void);
void set_cpu_halted(bool val);

#endif