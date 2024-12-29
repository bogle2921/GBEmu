#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "config.h"
// https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html

typedef enum{
    NONE, NOP, LD, INC, DEC, RLCA, ADD, RRCA, STOP,
    RLA, JR, RRA, DAA, CPL, SCF, CCF, HALT, ADC, SUB,
    SBC, AND, XOR, OR, CP, POP, JP, PUSH, RET, CB, CALL,
    RETI, LDH, JPHL, DI, EI, RST, ERR,
    // prefix CB
    CB_RLC, CB_RRC, CB_RL, CB_RR, CB_SLA, CB_SRA, CB_SWAP,
    CB_SRL, CB_BIT, CB_RES, CB_SET
} inst_type;

typedef enum{
    REG_NONE, REG_A, REG_F, REG_B,
    REG_C, REG_D, REG_E, REG_H, REG_L,
    REG_AF, REG_BC, REG_DE, REG_HL,
    REG_SP, REG_PC
} reg_name;

typedef enum{
    COND_NONE, COND_NOTZERO, COND_ZERO,
    COND_NOCARRY, COND_CARRY
} conditional;

typedef enum{
    P_NONE, // NOP HALT
    P_REG_D16, P_REG_REG, P_MEM_REG, P_REG,
    P_REG_D8, P_REG_MEM, P_REG_HL_INC, P_REG_HL_DEC,
    P_HL_INC_REG, P_HL_DEC_REG, P_REG_ADDR8, P_ADDR8_REG,
    P_HL_SP, P_D16, P_D8, P_D16_REG, P_MEM_D8, P_MEM, P_ADDR16_REG, P_REG_ADDR16
} placement;

struct instruction {
    inst_type i_type;
    reg_name reg1;
    reg_name reg2;
    conditional cond;
    u8 val;
    placement p_mode;
};

void cpu_step();
void cpu_init();

#endif
