#include "logger.h"
#include "cpu.h"

/*
THIS FILE IS ALSO A BIT MUCH.
OP-CODES WERE DONE IN A REALLY WEIRD WAY WITH SKELETON FIRST,
THEN ADDED MEAT ON BONES, BUT COMMENTED OUT SKELETON. TODO: CLEAN?
VERY MUCH DOUBT EVERYTHING IS WORKING AS GB INTENDS.
*/


// STATIC CPU INSTANCE
static cpu CPU = {0};

// OPCODE HANDLER TYPE
typedef void (*opcode_fn)(void);

// OPCODE TABLES
static opcode_fn opcode_table[256];
static opcode_fn cb_opcode_table[256];

// PRIVATE HELPER FUNCTIONS & STRUCTS
static u8* const REGISTERS[] = {
    &CPU.reg.b, &CPU.reg.c, &CPU.reg.d, &CPU.reg.e,
    &CPU.reg.h, &CPU.reg.l, NULL, &CPU.reg.a
};

static inline u8 read_byte(void) {
    u8 val = read_from_bus(CPU.reg.pc++);
    return val;
}

static inline u16 read_word(void) {
    u16 lo = read_from_bus(CPU.reg.pc);
    CPU.reg.pc++;
    u16 hi = read_from_bus(CPU.reg.pc);
    CPU.reg.pc++;
    return (hi << 8) | lo;
}

// GET/SET REG OR HL CONDITIONALLY
// TODO: DECIDED TO ADD THIS WHEN I GOT TO THE CB OPCODES, 
// PLS REPLACE IF/ELSE 0x06 IN OTHER OPS. CPU CYCLES WILL STILL BE SET CONDITIONALLY IN HANDLERS.
static u8 get_reg_or_hl(u8 reg_idx) {
    if (reg_idx == 0x06) {  // IS HL
        u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
        return read_from_bus(hl);
    }
    return *REGISTERS[reg_idx];
}

static void set_reg_or_hl(u8 reg_idx, u8 value) {
    if (reg_idx == 0x06) {  // IS HL
        u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
        write_to_bus(hl, value);
    } else {
        *REGISTERS[reg_idx] = value;
    }
}



// OPCODES BY CATEGORY -------------------------------------------------------------

// ------------------------------------- NO OP -------------------------------------

static void op_nop(void) {                           // NOP
    CPU.cycles = 4;
}

// ---------------------------------- 8-BIT LOADS ----------------------------------

// static void ld_r_r(void) { CPU.cycles = 4; }      // LD r,r - LOAD REG R1 WITH REG R2
static void ld_r_r(void) {
    u8 dst_reg = (CPU.opcode >> 3) & 0x07;
    u8 src_reg = CPU.opcode & 0x07;
    
    // Handle (HL) special case
    if (src_reg == 0x06) {
        u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
        *REGISTERS[dst_reg] = read_from_bus(hl);
        CPU.cycles = 8;
    } else if (dst_reg == 0x06) {
        u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
        write_to_bus(hl, *REGISTERS[src_reg]);
        CPU.cycles = 8;
    } else {
        *REGISTERS[dst_reg] = *REGISTERS[src_reg];
        CPU.cycles = 4;
    }
}
// static void ld_r_n(void) { CPU.cycles = 8; }      // LD r,n - LOAD REG R WITH IMMEDIATE VALUE N
// static void ld_r_hl(void) { CPU.cycles = 8; }     // LD r,(HL) - ALL HL CASES HANDLED INLINE
// static void ld_hl_r(void) { CPU.cycles = 8; }     // LD (HL),r - ALL HL CASES HANDLED INLINE
// static void ld_hl_n(void) { CPU.cycles = 12; }    // LD (HL),n - ALL HL CASES HANDLED INLINE
static void ld_r_n(void) {
    u8 dst_reg = (CPU.opcode >> 3) & 0x07;
    u8 n = read_byte();
    
    if (dst_reg == 0x06) {  // WE HANDLE MOST LOAD HL'S HERE
        u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
        write_to_bus(hl, n);
        CPU.cycles = 12;
    } else {
        *REGISTERS[dst_reg] = n;
        CPU.cycles = 8;
    }
}

// static void ld_a_bc(void) { CPU.cycles = 8; }     // LD A,(BC) - LOAD A FROM MEMORY POINTED AT BY BC
static void ld_a_bc(void) {
    u16 addr = MAKE_WORD(CPU.reg.b, CPU.reg.c); // BC
    CPU.reg.a = read_from_bus(addr);
    CPU.cycles = 8;
}
// static void ld_a_de(void) { CPU.cycles = 8; }     // LD A,(DE) - LOAD A FROM MEMORY POINTED AT BY DE
static void ld_a_de(void) {
    u16 addr = MAKE_WORD(CPU.reg.d, CPU.reg.e); // DE
    CPU.reg.a = read_from_bus(addr);
    CPU.cycles = 8;

}

// static void ld_a_nn(void) { CPU.cycles = 16; }    // LD A,(nn) - LOAD A FROM ABSOLUTE ADDR AT NN
static void ld_a_nn(void) {
    u16 addr = read_word(); // I THINK NN WOULD REFER TO THE IMMEDIATE 16 BIT VALUE
    CPU.reg.a = read_from_bus(addr);
    CPU.cycles = 16;
}

// static void ld_bc_a(void) { CPU.cycles = 8; }     // LD (BC),A - STORE A INTO MEMORY POINTED BY BC
static void ld_bc_a(void) {
    u16 addr = MAKE_WORD(CPU.reg.b, CPU.reg.c);
    write_to_bus(addr, CPU.reg.a);
    CPU.cycles = 8;
}

// static void ld_de_a(void) { CPU.cycles = 8; }     // LD (DE),A - STORE A INTO MEMORY POINTED BY DE
static void ld_de_a(void) {
    u16 addr = MAKE_WORD(CPU.reg.d, CPU.reg.e);
    write_to_bus(addr, CPU.reg.a);
    CPU.cycles = 8;
}

// static void ld_nn_a(void) { CPU.cycles = 16; }    // LD (nn),A - STORE A INTO DIRECT MEMORY ADDRESS
static void ld_nn_a(void) {
    u16 addr = read_word();
    write_to_bus(addr, CPU.reg.a);
    CPU.cycles = 16;
}

// static void ld_a_ff_n(void) { CPU.cycles = 12; }  // LDH A,(n) - LOAD A FROM HIGH MEM RNG OFFSET n (FF00+n)
static void ld_a_ff_n(void) {
    u8 offset = read_byte(); // N WOULD BE THE FIRST IMMEDIATE 8 BIT VALUE
    CPU.reg.a = read_from_bus(IO_START + offset); // HIGH MEM RNG STARTS AT FF00, SEE CONFIG.H
    CPU.cycles = 12;
}

// static void ld_ff_n_a(void) { CPU.cycles = 12; }  // LDH (n),A - STORE A INTO HIGH MEM RNG OFFSET n (FF00+n)
static void ld_ff_n_a(void) {
    u8 offset = read_byte();
    write_to_bus(IO_START + offset, CPU.reg.a);
    CPU.cycles = 12;
}

// static void ld_a_ff_c(void) { CPU.cycles = 8; }   // LD A,(C) - LOAD A FROM HIGH MEM RNG OFFSET C (FF00+C)
static void ld_a_ff_c(void) {
    CPU.reg.a = read_from_bus(IO_START + CPU.reg.c);
    CPU.cycles = 8;
}

// static void ld_ff_c_a(void) { CPU.cycles = 8; }   // LD (C),A - STORE A INTO HIGH MEM RNG OFFSET C (FF00+C)
static void ld_ff_c_a(void) {
    write_to_bus(IO_START + CPU.reg.c, CPU.reg.a);
    CPU.cycles = 8;
}

// static void ldi_hl_a(void) { CPU.cycles = 8; }    // LDI (HL),A - LOAD A *TO* HL AND *INCR* HL
static void ldi_hl_a(void) {
    u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
    write_to_bus(hl, CPU.reg.a);
    hl++;
    CPU.reg.h = GET_HIGH_BYTE(hl);
    CPU.reg.l = GET_LOW_BYTE(hl);
    CPU.cycles = 8;
}

// static void ldi_a_hl(void) { CPU.cycles = 8; }    // LDI A,(HL) - LOAD A *FROM* HL AND *INCR* HL
static void ldi_a_hl(void) {
    u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
    CPU.reg.a = read_from_bus(hl);
    hl++;
    CPU.reg.h = GET_HIGH_BYTE(hl);
    CPU.reg.l = GET_LOW_BYTE(hl);
    CPU.cycles = 8;
}

// static void ldd_hl_a(void) { CPU.cycles = 8; }    // LDD (HL),A - LOAD A *TO* HL AND *DECR* HL
static void ldd_hl_a(void) {
    u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
    write_to_bus(hl, CPU.reg.a);
    hl--;
    CPU.reg.h = GET_HIGH_BYTE(hl);
    CPU.reg.l = GET_LOW_BYTE(hl);
    CPU.cycles = 8;
}

// static void ldd_a_hl(void) { CPU.cycles = 8; }    // LDD A,(HL) - LOAD A *FROM* HL AND *DECR* HL
static void ldd_a_hl(void) {
    u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
    CPU.reg.a = read_from_bus(hl);
    hl--;
    CPU.reg.h = GET_HIGH_BYTE(hl);
    CPU.reg.l = GET_LOW_BYTE(hl);
    CPU.cycles = 8;
}

// --------------------------------- 16-BIT LOADS ----------------------------------

// static void ld_rr_nn(void) { CPU.cycles = 12; }   // LD rr,nn - LOAD 16 BIT NN (IMMEDIATE) VAL INTO REG PAIR
static void ld_rr_nn(void) {
    u8 reg_pair = (CPU.opcode >> 4) & 0x03; // CAN USE OPCODE TO GET TARGETS LIKE IN 8 BIT LOAD
                                            // SHIFT 4 AND MASK 0x03 FOR REGISTER PAIR TARGET
    u16 nn = read_word();                   // IMMEDIATE NN VAL COMES FROM TWO BYTES (WORD) READ IN
    
    switch(reg_pair) {
        case 0x00: // BC
            CPU.reg.b = GET_HIGH_BYTE(nn);
            CPU.reg.c = GET_LOW_BYTE(nn);
            break;
        case 0x01: // DE
            CPU.reg.d = GET_HIGH_BYTE(nn);
            CPU.reg.e = GET_LOW_BYTE(nn);
            break;
        case 0x02: // HL
            CPU.reg.h = GET_HIGH_BYTE(nn);
            CPU.reg.l = GET_LOW_BYTE(nn);
            break;
        case 0x03: // SP
            CPU.reg.sp = nn;
            break;
    }
    CPU.cycles = 12;
}

// static void ld_nn_sp(void) { CPU.cycles = 20; }   // LD (nn),SP - STORE SP AT ADDR POINTED TO BY IMMEDIATE VAL
static void ld_nn_sp(void) {
    u16 addr = read_word();
    write_to_bus(addr, GET_LOW_BYTE(CPU.reg.sp));
    write_to_bus(addr + 1, GET_HIGH_BYTE(CPU.reg.sp));
    CPU.cycles = 20;
}

// static void ld_sp_hl(void) { CPU.cycles = 8; }    // LD SP,HL - LOAD HL INTO SP
static void ld_sp_hl(void) {
    CPU.reg.sp = MAKE_WORD(CPU.reg.h, CPU.reg.l);
    CPU.cycles = 8;
}

// static void push_rr(void) { CPU.cycles = 16; }    // PUSH rr - PUSH REGISTER PAIR ONTO STACK
static void push_rr(void) {
    u8 reg_pair = (CPU.opcode >> 4) & 0x03;  // SAME DYNAMIC HANDLING FROM SHIFT/MASK OPCODE LIKE LD RR NN
    u8 high, low;                            // SHIFT 4 AND MASK 0x03 FOR REGISTER PAIR *SOURCE*
    
    switch(reg_pair) {
        case 0x00: // BC
            high = CPU.reg.b;
            low = CPU.reg.c;
            break;
        case 0x01: // DE
            high = CPU.reg.d;
            low = CPU.reg.e;
            break;
        case 0x02: // HL
            high = CPU.reg.h;
            low = CPU.reg.l;
            break;
        case 0x03: // AF
            high = CPU.reg.a;
            low = CPU.reg.f & 0xF0;         // ONLY UPPER BITS, MASK FLAGS
            break;
    }
    
    CPU.reg.sp--;
    write_to_bus(CPU.reg.sp, high);
    CPU.reg.sp--;
    write_to_bus(CPU.reg.sp, low);          // DECREMENT SP ADDR PER AND WRITE LOW/HIGH FROM REG PAIR TO SP
    
    CPU.cycles = 16;
}

// static void pop_rr(void) { CPU.cycles = 12; }     // POP rr
static void pop_rr(void) {
    u8 reg_pair = (CPU.opcode >> 4) & 0x03; // SAME DYNAMIC HANDLING FROM SHIFT/MASK OPCODE LIKE PUSH RR, BUT *TARGET*
    u8 low = read_from_bus(CPU.reg.sp);     // READ LOW/HIGH FROM SP, INCREMENT SP ADDR PER
    CPU.reg.sp++;
    u8 high = read_from_bus(CPU.reg.sp);
    CPU.reg.sp++;
    
    switch(reg_pair) {
        case 0x00: // BC
            CPU.reg.b = high;
            CPU.reg.c = low;
            break;
        case 0x01: // DE
            CPU.reg.d = high;
            CPU.reg.e = low;
            break;
        case 0x02: // HL
            CPU.reg.h = high;
            CPU.reg.l = low;
            break;
        case 0x03: // AF
            CPU.reg.a = high;
            CPU.reg.f = low & 0xF0;         // CAN ALWAYS ASSUME THESE LOWER 4 BITS ARE ZERO
            break;
    }
    
    CPU.cycles = 12;
}

// ----------------------------------- 8-BIT ALU -----------------------------------

// static void add_a_r(void) { CPU.cycles = 4; }     // ADD A,r - ADD A WITH REGISTER, UNLESS HL THEN VALUE AT HL
static void add_a_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value;
    if (reg_idx == 0x06) {  // WE HANDLE HL'S HERE
        value = read_from_bus(MAKE_WORD(CPU.reg.h, CPU.reg.l));
        CPU.cycles = 8;
    } else {
        value =  *REGISTERS[reg_idx];
        CPU.cycles = 4;
    }
    u16 result = CPU.reg.a + value;
    
    SET_FLAG_Z((result & 0xFF) == 0);
    SET_FLAG_N(0);
    SET_FLAG_H((CPU.reg.a & 0x0F) + (value & 0x0F) > 0x0F);
    SET_FLAG_C(result > 0xFF);
    
    CPU.reg.a = result & 0xFF;
}

// static void add_a_n(void) { CPU.cycles = 8; }     // ADD A,n - ADD A WITH IMMEDIATE VALUE N
static void add_a_n(void) {
    u8 value = read_byte();
    u16 result = CPU.reg.a + value;
    
    SET_FLAG_Z((result & 0xFF) == 0);
    SET_FLAG_N(0);
    SET_FLAG_H((CPU.reg.a & 0x0F) + (value & 0x0F) > 0x0F);
    SET_FLAG_C(result > 0xFF);
    
    CPU.reg.a = result & 0xFF;
    CPU.cycles = 8;
}

// static void adc_a_r(void) { CPU.cycles = 4; }     // ADC A,r - ADD WITH CARRY
static void adc_a_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value;
    if (reg_idx == 0x06) {  // WE HANDLE HL'S HERE
        value = read_from_bus(MAKE_WORD(CPU.reg.h, CPU.reg.l));
        CPU.cycles = 8;
    } else {
        value =  *REGISTERS[reg_idx];
        CPU.cycles = 4;
    }
    u8 carry = GET_FLAG_C;
    u16 result = CPU.reg.a + value + carry;
    
    SET_FLAG_Z((result & 0xFF) == 0);
    SET_FLAG_N(0);
    SET_FLAG_H((CPU.reg.a & 0x0F) + (value & 0x0F) + carry > 0x0F);
    SET_FLAG_C(result > 0xFF);
    
    CPU.reg.a = result & 0xFF;
}

// static void adc_a_n(void) { CPU.cycles = 8; }     // ADC A,n - ADD IMMEDIATE WITH CARRY
static void adc_a_n(void) {
    u8 value = read_byte();
    u8 carry = GET_FLAG_C;
    u16 result = CPU.reg.a + value + carry;
    
    SET_FLAG_Z((result & 0xFF) == 0);
    SET_FLAG_N(0);
    SET_FLAG_H((CPU.reg.a & 0x0F) + (value & 0x0F) + carry > 0x0F);
    SET_FLAG_C(result > 0xFF);
    
    CPU.reg.a = result & 0xFF;
    CPU.cycles = 8;
}

// static void sub_a_r(void) { CPU.cycles = 4; }     // SUB A,r - SUBTRACT FROM A
static void sub_a_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value;
    if (reg_idx == 0x06) {  // WE HANDLE HL'S HERE
        value = read_from_bus(MAKE_WORD(CPU.reg.h, CPU.reg.l));
        CPU.cycles = 8;
    } else {
        value =  *REGISTERS[reg_idx];
        CPU.cycles = 4;
    }
    u16 result = CPU.reg.a - value;
    
    SET_FLAG_Z((result & 0xFF) == 0);
    SET_FLAG_N(1);
    SET_FLAG_H((CPU.reg.a & 0x0F) < (value & 0x0F));
    SET_FLAG_C(result > 0xFF);
    
    CPU.reg.a = result & 0xFF;
}

// static void sub_a_n(void) { CPU.cycles = 8; }     // SUB A,n - SUBTRACT IMMEDIATE FROM A
static void sub_a_n(void) {
    u8 value = read_byte();
    u16 result = CPU.reg.a - value;
    
    SET_FLAG_Z((result & 0xFF) == 0);
    SET_FLAG_N(1);
    SET_FLAG_H((CPU.reg.a & 0x0F) < (value & 0x0F));
    SET_FLAG_C(result > 0xFF);
    
    CPU.reg.a = result & 0xFF;
    CPU.cycles = 8;
}

// static void sbc_a_r(void) { CPU.cycles = 4; }     // SBC A,r - SUBTRACT WITH CARRY FROM A
static void sbc_a_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value;
    if (reg_idx == 0x06) {  // WE HANDLE HL'S HERE
        value = read_from_bus(MAKE_WORD(CPU.reg.h, CPU.reg.l));
        CPU.cycles = 8;
    } else {
        value =  *REGISTERS[reg_idx];
        CPU.cycles = 4;
    }
    u8 carry = GET_FLAG_C;
    u16 result = CPU.reg.a - value - carry;
    
    SET_FLAG_Z((result & 0xFF) == 0);
    SET_FLAG_N(1);
    SET_FLAG_H((CPU.reg.a & 0x0F) < ((value & 0x0F) + carry));
    SET_FLAG_C(result > 0xFF);
    
    CPU.reg.a = result & 0xFF;
}

// static void sbc_a_n(void) { CPU.cycles = 8; }     // SBC A,n - SUBTRACT IMMEDIATE WITH CARRY FROM A
static void sbc_a_n(void) {
    u8 value = read_byte();
    u8 carry = GET_FLAG_C;
    u16 result = CPU.reg.a - value - carry;
    
    SET_FLAG_Z((result & 0xFF) == 0);
    SET_FLAG_N(1);
    SET_FLAG_H((CPU.reg.a & 0x0F) < ((value & 0x0F) + carry));
    SET_FLAG_C(result > 0xFF);
    
    CPU.reg.a = result & 0xFF;
    CPU.cycles = 8;
}

// static void and_r(void) { CPU.cycles = 4; }       // AND r - LOGICAL AND WITH REG
static void and_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value;
    if (reg_idx == 0x06) {  // WE HANDLE HL'S HERE
        value = read_from_bus(MAKE_WORD(CPU.reg.h, CPU.reg.l));
        CPU.cycles = 8;
    } else {
        value =  *REGISTERS[reg_idx];
        CPU.cycles = 4;
    }
    
    CPU.reg.a &= value;
    
    SET_FLAG_Z(CPU.reg.a == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(1);
    SET_FLAG_C(0);
}

// static void and_n(void) { CPU.cycles = 8; }       // AND n - LOGICAL AND WITH IMMEDIATE N
static void and_n(void) {
    u8 value = read_byte();
    
    CPU.reg.a &= value;
    
    SET_FLAG_Z(CPU.reg.a == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(1);
    SET_FLAG_C(0);
    
    CPU.cycles = 8;
}

// static void xor_r(void) { CPU.cycles = 4; }       // XOR r - LOGICAL XOR WITH REG
static void xor_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value;
    if (reg_idx == 0x06) {  // WE HANDLE HL'S HERE
        value = read_from_bus(MAKE_WORD(CPU.reg.h, CPU.reg.l));
        CPU.cycles = 8;
    } else {
        value =  *REGISTERS[reg_idx];
        CPU.cycles = 4;
    }

    CPU.reg.a ^= value;
    
    SET_FLAG_Z(CPU.reg.a == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(0);
}

// static void xor_n(void) { CPU.cycles = 8; }       // XOR n - LOGICAL XOR WITH IMMEDIATE N
static void xor_n(void) {
    u8 value = read_byte();
    
    CPU.reg.a ^= value;
    
    SET_FLAG_Z(CPU.reg.a == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(0);
    
    CPU.cycles = 8;
}

// static void or_r(void) { CPU.cycles = 4; }        // OR r - LOGICAL OR WITH REG
static void or_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value;
    if (reg_idx == 0x06) {  // WE HANDLE HL'S HERE
        value = read_from_bus(MAKE_WORD(CPU.reg.h, CPU.reg.l));
        CPU.cycles = 8;
    } else {
        value =  *REGISTERS[reg_idx];
        CPU.cycles = 4;
    }
    
    CPU.reg.a |= value;
    
    SET_FLAG_Z(CPU.reg.a == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(0);
}

// static void or_n(void) { CPU.cycles = 8; }        // OR n - LOGICAL OR WITH IMMEDIATE N
static void or_n(void) {
    u8 value = read_byte();
    
    CPU.reg.a |= value;
    
    SET_FLAG_Z(CPU.reg.a == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(0);
    
    CPU.cycles = 8;
}

// static void cp_r(void) { CPU.cycles = 4; }        // CP r - COMPARE WITH REG (SUBTRACT BUT DON'T STORE)
static void cp_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value;
    if (reg_idx == 0x06) {  // WE HANDLE HL'S HERE
        value = read_from_bus(MAKE_WORD(CPU.reg.h, CPU.reg.l));
        CPU.cycles = 8;
    } else {
        value =  *REGISTERS[reg_idx];
        CPU.cycles = 4;
    }
    u16 result = CPU.reg.a - value;
    
    SET_FLAG_Z((result & 0xFF) == 0);
    SET_FLAG_N(1);
    SET_FLAG_H((CPU.reg.a & 0x0F) < (value & 0x0F));
    SET_FLAG_C(result > 0xFF);
}

// static void cp_n(void) { CPU.cycles = 8; }        // CP n - COMPARE WITH IMMEDIATE N (SUBTRACT BUT DON'T STORE)
static void cp_n(void) {
    u8 value = read_byte();
    u16 result = CPU.reg.a - value;
    
    SET_FLAG_Z((result & 0xFF) == 0);
    SET_FLAG_N(1);
    SET_FLAG_H((CPU.reg.a & 0x0F) < (value & 0x0F));
    SET_FLAG_C(result > 0xFF);
    
    CPU.cycles = 8;
}

// static void inc_r(void) { CPU.cycles = 4; }       // INC r - INCR REG BY 1 UNLESS HL (THEN VAL AT HL)
static void inc_r(void) {
    u8 reg_idx = (CPU.opcode >> 3) & 0x07; // SHIFT/MASK TO COVER MULTI OPCODES
    u8 *reg = REGISTERS[reg_idx];
    
    if (reg_idx == 0x06) {  // WE HANDLE HL'S HERE
        u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
        u8 value = read_from_bus(MAKE_WORD(CPU.reg.h, CPU.reg.l));
        u8 result = value + 1;
        
        SET_FLAG_Z(result == 0);
        SET_FLAG_N(0);
        SET_FLAG_H((value & 0x0F) == 0x0F);
        
        write_to_bus(hl, result);
        CPU.cycles = 12;
    } else {
        u8 result = *reg + 1;
        
        SET_FLAG_Z(result == 0);
        SET_FLAG_N(0);
        SET_FLAG_H((*reg & 0x0F) == 0x0F);
        
        *reg = result;
        CPU.cycles = 4;
    }
}

// static void dec_r(void) { CPU.cycles = 4; }       // DEC r - DECR REG BY 1 UNLESS HL (THEN VAL AT HL)
static void dec_r(void) {
    u8 reg_idx = (CPU.opcode >> 3) & 0x07; // SHIFT/MASK TO COVER MULTI OPCODES
    u8 *reg = REGISTERS[reg_idx];
    
    if (reg_idx == 0x06) {  // WE HANDLE HL'S HERE
        u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
        u8 value = read_from_bus(hl);
        u8 result = value - 1;
        
        SET_FLAG_Z(result == 0);
        SET_FLAG_N(1);
        SET_FLAG_H((value & 0x0F) == 0x00);
        
        write_to_bus(hl, result);
        CPU.cycles = 12;
    } else {
        u8 result = *reg - 1;
        
        SET_FLAG_Z(result == 0);
        SET_FLAG_N(1);
        SET_FLAG_H((*reg & 0x0F) == 0x00);
        
        *reg = result;
        CPU.cycles = 4;
    }
}

// ---------------------------------- 16-BIT ALU -----------------------------------

// static void add_hl_rr(void) { CPU.cycles = 8; }   // ADD HL,rr - ADD HL WITH 16-BIT REG
static void add_hl_rr(void) {
    u8 reg_pair = (CPU.opcode >> 4) & 0x03;
    u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
    u16 value;

    // GET VALUE FROM REG PAIR
    if (reg_pair == 0x00) {        // BC
        value = MAKE_WORD(CPU.reg.b, CPU.reg.c);
    } else if (reg_pair == 0x01) { // DE
        value = MAKE_WORD(CPU.reg.d, CPU.reg.e);
    } else if (reg_pair == 0x02) { // HL
        value = hl;
    } else {                       // SP
        value = CPU.reg.sp;
    }

    u32 result = hl + value;
    
    SET_FLAG_N(0);
    SET_FLAG_H((hl & 0x0FFF) + (value & 0x0FFF) > 0x0FFF);
    SET_FLAG_C(result > 0xFFFF);
    
    // STORE RESULT BACK IN HL
    CPU.reg.h = GET_HIGH_BYTE(result);
    CPU.reg.l = GET_LOW_BYTE(result);
    
    CPU.cycles = 8;
}

// static void inc_rr(void) { CPU.cycles = 8; }      // INC rr - INCR 16-BIT REG
static void inc_rr(void) {
    u8 reg_pair = (CPU.opcode >> 4) & 0x03;
    
    if (reg_pair == 0x00) {        // BC
        u16 bc = MAKE_WORD(CPU.reg.b, CPU.reg.c);
        bc++;
        CPU.reg.b = GET_HIGH_BYTE(bc);
        CPU.reg.c = GET_LOW_BYTE(bc);
    } else if (reg_pair == 0x01) { // DE
        u16 de = MAKE_WORD(CPU.reg.d, CPU.reg.e);
        de++;
        CPU.reg.d = GET_HIGH_BYTE(de);
        CPU.reg.e = GET_LOW_BYTE(de);
    } else if (reg_pair == 0x02) { // HL
        u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
        hl++;
        CPU.reg.h = GET_HIGH_BYTE(hl);
        CPU.reg.l = GET_LOW_BYTE(hl);
    } else {                       // SP
        CPU.reg.sp++;
    }
    
    CPU.cycles = 8;
}

// static void dec_rr(void) { CPU.cycles = 8; }      // DEC rr - DECR 16-BIT REG
static void dec_rr(void) {
    u8 reg_pair = (CPU.opcode >> 4) & 0x03;
    
    if (reg_pair == 0x00) {        // BC
        u16 bc = MAKE_WORD(CPU.reg.b, CPU.reg.c);
        bc--;
        CPU.reg.b = GET_HIGH_BYTE(bc);
        CPU.reg.c = GET_LOW_BYTE(bc);
    } else if (reg_pair == 0x01) { // DE
        u16 de = MAKE_WORD(CPU.reg.d, CPU.reg.e);
        de--;
        CPU.reg.d = GET_HIGH_BYTE(de);
        CPU.reg.e = GET_LOW_BYTE(de);
    } else if (reg_pair == 0x02) { // HL
        u16 hl = MAKE_WORD(CPU.reg.h, CPU.reg.l);
        hl--;
        CPU.reg.h = GET_HIGH_BYTE(hl);
        CPU.reg.l = GET_LOW_BYTE(hl);
    } else {                       // SP
        CPU.reg.sp--;
    }
    
    CPU.cycles = 8;
}

// static void add_sp_d(void) { CPU.cycles = 16; }   // ADD SP,d - ADD SIGNED IMMEDIATE TO SP
static void add_sp_d(void) {
    // READ SIGNED IMMEDIATE VALUE
    i8 value = (i8)read_byte();
    u16 result = CPU.reg.sp + value;
    
    SET_FLAG_Z(0);
    SET_FLAG_N(0);
    
    // HALF CARRY IS FROM BIT 3 FOR SP + d
    if (value >= 0) {
        SET_FLAG_H((CPU.reg.sp & 0x0F) + (value & 0x0F) > 0x0F);
        SET_FLAG_C((CPU.reg.sp & 0xFF) + (value & 0xFF) > 0xFF);
    } else {
        SET_FLAG_H((CPU.reg.sp & 0x0F) + (value & 0x0F) < 0);
        SET_FLAG_C((CPU.reg.sp & 0xFF) + (value & 0xFF) < 0);
    }
    
    CPU.reg.sp = result;
    CPU.cycles = 16;
}

// static void ld_hl_sp_d(void) { CPU.cycles = 12; } // LD HL,SP+d - LOAD HL WITH SP + SIGNED IMMEDIATE
static void ld_hl_sp_d(void) {
    i8 value = (i8)read_byte();
    u16 result = CPU.reg.sp + value;
    
    SET_FLAG_Z(0);
    SET_FLAG_N(0);
    
    // HALF CARRY IS FROM BIT 3 FOR SP + d
    if (value >= 0) {
        SET_FLAG_H((CPU.reg.sp & 0x0F) + (value & 0x0F) > 0x0F);
        SET_FLAG_C((CPU.reg.sp & 0xFF) + (value & 0xFF) > 0xFF);
    } else {
        SET_FLAG_H((CPU.reg.sp & 0x0F) + (value & 0x0F) < 0);
        SET_FLAG_C((CPU.reg.sp & 0xFF) + (value & 0xFF) < 0);
    }
    
    CPU.reg.h = GET_HIGH_BYTE(result);
    CPU.reg.l = GET_LOW_BYTE(result);
    CPU.cycles = 12;
}

// ------------------------------- ROTATES AND SHIFTS ------------------------------

// static void rlca(void) { CPU.cycles = 4; }        // RLCA - ROTATE A LEFT WITH BIT 7 TO CARRY
static void rlca(void) {
    // SAVE BIT 7 *BEFORE* ROTATION
    u8 bit7 = (CPU.reg.a & 0x80) >> 7;
    
    // ROTATE LEFT W/ BIT 7
    CPU.reg.a = (CPU.reg.a << 1) | bit7;
    
    SET_FLAG_Z(0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(bit7);
    
    CPU.cycles = 4;
}

// static void rla(void) { CPU.cycles = 4; }         // RLA - ROTATE A LEFT THROUGH CARRY
static void rla(void) {
    // SAVE FLAG C AND BIT 7 *BEFORE* ROTATION
    u8 old_carry = GET_FLAG_C;
    u8 bit7 = (CPU.reg.a & 0x80) >> 7;
    
    // ROTATE LEFT W/ CARRY
    CPU.reg.a = (CPU.reg.a << 1) | old_carry;
    
    SET_FLAG_Z(0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(bit7);
    
    CPU.cycles = 4;
}

// static void rrca(void) { CPU.cycles = 4; }        // RRCA - ROTATE A RIGHT WITH BIT 0 TO CARRY
static void rrca(void) {
    // SAVE BIT 0 *BEFORE* ROTATION
    u8 bit0 = CPU.reg.a & 0x01;
    
    // ROTATE RIGHT W/ BIT 0
    CPU.reg.a = (CPU.reg.a >> 1) | (bit0 << 7);
    
    SET_FLAG_Z(0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(bit0);
    
    CPU.cycles = 4;
}

// static void rra(void) { CPU.cycles = 4; }         // RRA - ROTATE A RIGHT THROUGH CARRY
static void rra(void) {
    // SAVE FLAG C AND BIT 0 *BEFORE* ROTATION
    u8 old_carry = GET_FLAG_C;
    u8 bit0 = CPU.reg.a & 0x01;
    
    // ROTATE RIGHT W/ CARRY
    CPU.reg.a = (CPU.reg.a >> 1) | (old_carry << 7);
    
    SET_FLAG_Z(0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(bit0);
    
    CPU.cycles = 4;
}

// ------------------------------ CB PREFIX OPERATIONS -----------------------------

// static void rlc_r(void) { CPU.cycles = 8; }       // RLC r - ROTATE LEFT WITH COPY TO CARRY
static void rlc_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value = get_reg_or_hl(reg_idx); // BROKE OUT HL IF/ELSE TO HELPER FN, NEED TO REF OTHERS AS WELL
    u8 bit7 = (value & 0x80) >> 7;
    
    value = (value << 1) | bit7;
    set_reg_or_hl(reg_idx, value);
    
    SET_FLAG_Z(value == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(bit7);
    
    // WILL BEGIN SETTING HL CPU CYCLES LIKE THIS
    CPU.cycles = (reg_idx == 0x06) ? 16 : 8;
}

// static void rl_r(void) { CPU.cycles = 8; }        // RL r - ROTATE LEFT THROUGH CARRY
static void rl_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value = get_reg_or_hl(reg_idx);
    u8 old_carry = GET_FLAG_C;
    u8 bit7 = (value & 0x80) >> 7;
    
    value = (value << 1) | old_carry;
    set_reg_or_hl(reg_idx, value);
    
    SET_FLAG_Z(value == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(bit7);
    
    CPU.cycles = (reg_idx == 0x06) ? 16 : 8;
}

// static void rrc_r(void) { CPU.cycles = 8; }       // RRC r - ROTATE RIGHT WITH COPY TO CARRY
static void rrc_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value = get_reg_or_hl(reg_idx);
    u8 bit0 = value & 0x01;
    
    value = (value >> 1) | (bit0 << 7);
    set_reg_or_hl(reg_idx, value);
    
    SET_FLAG_Z(value == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(bit0);
    
    CPU.cycles = (reg_idx == 0x06) ? 16 : 8;
}

// static void rr_r(void) { CPU.cycles = 8; }        // RR r - ROTATE RIGHT THROUGH CARRY
static void rr_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value = get_reg_or_hl(reg_idx);
    u8 old_carry = GET_FLAG_C;
    u8 bit0 = value & 0x01;
    
    value = (value >> 1) | (old_carry << 7);
    set_reg_or_hl(reg_idx, value);
    
    SET_FLAG_Z(value == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(bit0);
    
    CPU.cycles = (reg_idx == 0x06) ? 16 : 8;
}

// static void sla_r(void) { CPU.cycles = 8; }       // SLA r - SHIFT LEFT ARITHMETIC (BIT ZERO = 0)
static void sla_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value = get_reg_or_hl(reg_idx);
    u8 bit7 = (value & 0x80) >> 7;
    
    value = value << 1;
    set_reg_or_hl(reg_idx, value);
    
    SET_FLAG_Z(value == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(bit7);
    
    CPU.cycles = (reg_idx == 0x06) ? 16 : 8;
}

// static void sra_r(void) { CPU.cycles = 8; }       // SRA r - SHIFT RIGHT ARITHMETIC (BIT 7 UNCHANGED)
static void sra_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value = get_reg_or_hl(reg_idx);
    u8 bit0 = value & 0x01;
    u8 bit7 = value & 0x80;
    
    value = (value >> 1) | bit7;
    set_reg_or_hl(reg_idx, value);
    
    SET_FLAG_Z(value == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(bit0);
    
    CPU.cycles = (reg_idx == 0x06) ? 16 : 8;
}

// static void swap_r(void) { CPU.cycles = 8; }      // SWAP r - SWAP NIBBLES
static void swap_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value = get_reg_or_hl(reg_idx);
    
    value = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);
    set_reg_or_hl(reg_idx, value);
    
    SET_FLAG_Z(value == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(0);
    
    CPU.cycles = (reg_idx == 0x06) ? 16 : 8;
}

// static void srl_r(void) { CPU.cycles = 8; }       // SRL r - SHIFT RIGHT LOGICAL (BIT 7 = 0)
static void srl_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 value = get_reg_or_hl(reg_idx);
    u8 bit0 = value & 0x01;
    
    value = value >> 1;
    set_reg_or_hl(reg_idx, value);
    
    SET_FLAG_Z(value == 0);
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(bit0);
    
    CPU.cycles = (reg_idx == 0x06) ? 16 : 8;
}

// static void bit_n_r(void) { CPU.cycles = 8; }     // BIT n,r - TEST BIT, SET Z FLAG IF BIT N IS 0, ELSE CLEARS
static void bit_n_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 bit_num = (CPU.opcode >> 3) & 0x07;
    u8 value = get_reg_or_hl(reg_idx);
    
    SET_FLAG_Z(!(value & (1 << bit_num)));
    SET_FLAG_N(0);
    SET_FLAG_H(1);
    
    CPU.cycles = (reg_idx == 0x06) ? 12 : 8;
}

// static void set_n_r(void) { CPU.cycles = 8; }     // SET n,r - SET BIT N IN REG TO 1, VAL IS LOG OR W/ 1 AT POS N
static void set_n_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 bit_num = (CPU.opcode >> 3) & 0x07;
    u8 value = get_reg_or_hl(reg_idx);
    
    value |= (1 << bit_num);
    set_reg_or_hl(reg_idx, value);
    
    CPU.cycles = (reg_idx == 0x06) ? 16 : 8;
}

// static void res_n_r(void) { CPU.cycles = 8; }     // RES n,r - RESET BIT AT REG TO 0, VAL IS LOG AND w/ 0 AT POS N
static void res_n_r(void) {
    u8 reg_idx = CPU.opcode & 0x07;
    u8 bit_num = (CPU.opcode >> 3) & 0x07;
    u8 value = get_reg_or_hl(reg_idx);
    
    value &= ~(1 << bit_num); // SQUIGGLY INVERTS BITS
    set_reg_or_hl(reg_idx, value);
    
    CPU.cycles = (reg_idx == 0x06) ? 16 : 8;
}

// ----------------------------------- CPU CONTROL ---------------------------------

// static void ccf(void) { CPU.cycles = 4; }         // CCF - FLIP C FLAG
static void ccf(void) {
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(!GET_FLAG_C);
    CPU.cycles = 4;
}

// static void scf(void) { CPU.cycles = 4; }         // SCF - SET C FLAG
static void scf(void) {
    SET_FLAG_N(0);
    SET_FLAG_H(0);
    SET_FLAG_C(1);
    CPU.cycles = 4;
}

// static void halt(void) { CPU.cycles = 4; }        // HALT - OBVIOUSLY HALTS THE CPU
static void halt(void) {
    CPU.halted = 1;
    CPU.cycles = 4;
}

// static void stop(void) { CPU.cycles = 4; }        // STOP - "STOPS" POSSIBLY UNTIL BTN PRESS, LIKE SLEEP MODE
static void stop(void) {
    CPU.stopped = 1;
    CPU.cycles = 4;
}

// static void di(void) { CPU.cycles = 4; }          // DI - DISABLE INTERRUPTS
static void di(void) {
    CPU.ime = 0;  // INTERRUPT MASTER ENABLE FLAG
    CPU.cycles = 4;
}

// static void ei(void) { CPU.cycles = 4; }          // EI - ENABLE INTERRUPTS
static void ei(void) {
    CPU.ime_scheduled = 1;
    CPU.cycles = 4;
}

// -------------------------------------- JUMPS ------------------------------------

// static void jp_nn(void) { CPU.cycles = 16; }      // JP nn - JUMP TO ABSOLUTE ADDRESS NN
static void jp_nn(void) {
    u16 addr = read_word();
    CPU.reg.pc = addr;
    CPU.cycles = 16;
}

// static void jp_cc_nn(void) { CPU.cycles = 12; }   // JP cc,nn - CONDITIONAL JUMP TO ABSOLUTE ADDRESS NN
static void jp_cc_nn(void) {
    u8 condition = (CPU.opcode >> 3) & 0x03;
    u16 addr = read_word();
    bool jump = false;

    switch(condition) { // TODO: REPLACE WITH HASH MAP, SWITCHES AND IFS ARE FOR THE WEAK
        case 0: jump = !GET_FLAG_Z; break;    // NZ
        case 1: jump = GET_FLAG_Z;  break;    // Z
        case 2: jump = !GET_FLAG_C; break;    // NC
        case 3: jump = GET_FLAG_C;  break;    // C
    }

    if (jump) {
        CPU.reg.pc = addr;
        CPU.cycles = 16;
    } else {
        CPU.cycles = 12;
    }
}

// static void jp_hl(void) { CPU.cycles = 4; }       // JP (HL) - JUMP TO HL
static void jp_hl(void) {
    CPU.reg.pc = MAKE_WORD(CPU.reg.h, CPU.reg.l);
    CPU.cycles = 4;
}

// static void jr_n(void) { CPU.cycles = 12; }       // JR n - RELATIVE JUMP BY SIGNED IMMEDIATE N
static void jr_n(void) {
    i8 offset = (i8)read_byte();
    CPU.reg.pc += offset;
    CPU.cycles = 12;
}

// static void jr_cc_n(void) { CPU.cycles = 8; }     // JR cc,n - CONDITIONAL RELATIVE JUMP BY SIGNED IMMEDIATE N
static void jr_cc_n(void) {
    u8 condition = (CPU.opcode >> 3) & 0x03;
    i8 offset = (i8)read_byte();
    bool jump = false;

    switch(condition) { // TODO: REPLACE WITH HASH MAP, SWITCHES AND IFS ARE FOR THE WEAK
        case 0: jump = !GET_FLAG_Z; break;    // NZ
        case 1: jump = GET_FLAG_Z;  break;    // Z
        case 2: jump = !GET_FLAG_C; break;    // NC
        case 3: jump = GET_FLAG_C;  break;    // C
    }

    if (jump) {
        CPU.reg.pc += offset;
        CPU.cycles = 12;
    } else {
        CPU.cycles = 8;
    }
}

// -------------------------------------- CALLS ------------------------------------

// static void call_nn(void) { CPU.cycles = 24; }    // CALL nn - CALL ABSOLUTE ADDRESS NN
static void call_nn(void) {
    u16 addr = read_word();
    
    // PUSH CURRENT PC ONTO STACK
    CPU.reg.sp--;
    write_to_bus(CPU.reg.sp, GET_HIGH_BYTE(CPU.reg.pc));
    CPU.reg.sp--;
    write_to_bus(CPU.reg.sp, GET_LOW_BYTE(CPU.reg.pc));
    
    // JUMP TO ADDRESS
    CPU.reg.pc = addr;
    CPU.cycles = 24;
}

// static void call_cc_nn(void) { CPU.cycles = 12; } // CALL cc,nn - CONDITIONAL CALL TO ABSOLUTE ADDRESS NN
static void call_cc_nn(void) {
    u8 condition = (CPU.opcode >> 3) & 0x03;
    u16 addr = read_word();
    bool jump = false;
    
    switch(condition) {
        case 0: jump = !GET_FLAG_Z; break;    // NZ
        case 1: jump = GET_FLAG_Z;  break;    // Z
        case 2: jump = !GET_FLAG_C; break;    // NC
        case 3: jump = GET_FLAG_C;  break;    // C
    }
    
    if (jump) { // ONLY PUSH CURRENT PC ONTO STACK IF JUMP
        CPU.reg.sp--;
        write_to_bus(CPU.reg.sp, GET_HIGH_BYTE(CPU.reg.pc));
        CPU.reg.sp--;
        write_to_bus(CPU.reg.sp, GET_LOW_BYTE(CPU.reg.pc));
        
        // JUMP TO ADDRESS
        CPU.reg.pc = addr;
        CPU.cycles = 24;
    } else {
        CPU.cycles = 12;
    }
}

// ------------------------------------ RESTARTS -----------------------------------

// static void rst_n(void) { CPU.cycles = 16; }      // RST n - RESTART, CALL TO FIXED ADDRESS
static void rst_n(void) {
    // RST'S ARE AT FIXED ADDR
    static const u16 RST_VECTORS[] = {
        0x00,  // RST 00h (0xC7)
        0x08,  // RST 08h (0xCF) 
        0x10,  // RST 10h (0xD7)
        0x18,  // RST 18h (0xDF)
        0x20,  // RST 20h (0xE7)
        0x28,  // RST 28h (0xEF)
        0x30,  // RST 30h (0xF7)
        0x38   // RST 38h (0xFF)
    };

    // INDEX FROM OPCODE
    u8 vector_idx = ((CPU.opcode & 0x38) >> 3);
    u16 target_addr = RST_VECTORS[vector_idx];
    
    // PUSH CURRENT PC ONTO STACK
    CPU.reg.sp--;
    write_to_bus(CPU.reg.sp, GET_HIGH_BYTE(CPU.reg.pc));
    CPU.reg.sp--;
    write_to_bus(CPU.reg.sp, GET_LOW_BYTE(CPU.reg.pc));
    
    CPU.reg.pc = target_addr;
    CPU.cycles = 16;
}

// ------------------------------------ RETURNS ------------------------------------

// static void ret(void) { CPU.cycles = 16; }        // RET - LETS US USE SUBROUTINES AND GO BACK TO CALLER
static void ret(void) {
   // POP ADDRESS FROM STACK
   u8 low = read_from_bus(CPU.reg.sp);
   CPU.reg.sp++;
   u8 high = read_from_bus(CPU.reg.sp);
   CPU.reg.sp++;
   
   // JUMP TO POPPED ADDRESS
   CPU.reg.pc = MAKE_WORD(high, low);
   CPU.cycles = 16;
}

// static void ret_cc(void) { CPU.cycles = 8; }      // RET cc - CONDITIONAL RETURN FROM SUBROUTINE
static void ret_cc(void) {
   u8 condition = (CPU.opcode >> 3) & 0x03;
   bool do_ret = false;
   
   switch(condition) {
       case 0: do_ret = !GET_FLAG_Z; break;    // NZ
       case 1: do_ret = GET_FLAG_Z;  break;    // Z
       case 2: do_ret = !GET_FLAG_C; break;    // NC
       case 3: do_ret = GET_FLAG_C;  break;    // C
   }
   
   if (do_ret) {
       // POP ADDRESS FROM STACK
       u8 low = read_from_bus(CPU.reg.sp);
       CPU.reg.sp++;
       u8 high = read_from_bus(CPU.reg.sp);
       CPU.reg.sp++;
       
       // JUMP TO POPPED ADDRESS
       CPU.reg.pc = MAKE_WORD(high, low);
       CPU.cycles = 20;
   } else {
       CPU.cycles = 8;
   }
}

// static void reti(void) { CPU.cycles = 16; }       // RETI - RETURN FROM INTERUPT, DO WE REENABLE IME FLAG?
static void reti(void) {
   // POP ADDRESS FROM STACK
   u8 low = read_from_bus(CPU.reg.sp);
   CPU.reg.sp++;
   u8 high = read_from_bus(CPU.reg.sp);
   CPU.reg.sp++;
   
   // JUMP TO POPPED ADDRESS AND ENABLE INTERRUPTS
   CPU.reg.pc = MAKE_WORD(high, low);
   CPU.ime = 1; // GOING TO RE-ENABLE FOR NOW, MIGHT NEED TO DISABLE
   CPU.cycles = 16;
}

// TABLE INITIALIZATION ------------------------------------------------------------

static void init_tables(void) {
    // INIT EVERYTHING TO NOP FIRST
    for (int i = 0; i < 256; i++) {
        opcode_table[i] = op_nop;
        cb_opcode_table[i] = op_nop;
    }

    // 8-BIT LOAD INSTRUCTIONS
    for (int i = 0x40; i <= 0x7F; i++) {
        opcode_table[i] = ld_r_r;  // ALL REG-REG LOADS
    }

    opcode_table[0x06] = ld_r_n;      // LD B,n
    opcode_table[0x0E] = ld_r_n;      // LD C,n
    opcode_table[0x16] = ld_r_n;      // LD D,n
    opcode_table[0x1E] = ld_r_n;      // LD E,n
    opcode_table[0x26] = ld_r_n;      // LD H,n
    opcode_table[0x2E] = ld_r_n;      // LD L,n
    opcode_table[0x3E] = ld_r_n;      // LD A,n
    opcode_table[0x0A] = ld_a_bc;     // LD A,(BC)
    opcode_table[0x1A] = ld_a_de;     // LD A,(DE)
    opcode_table[0xFA] = ld_a_nn;     // LD A,(nn)
    opcode_table[0x02] = ld_bc_a;     // LD (BC),A
    opcode_table[0x12] = ld_de_a;     // LD (DE),A
    opcode_table[0xEA] = ld_nn_a;     // LD (nn),A
    opcode_table[0x36] = ld_r_n;      // LD (HL),n
    opcode_table[0xF0] = ld_a_ff_n;   // LDH A,(n)
    opcode_table[0xE0] = ld_ff_n_a;   // LDH (n),A
    opcode_table[0xF2] = ld_a_ff_c;   // LD A,(C)
    opcode_table[0xE2] = ld_ff_c_a;   // LD (C),A
    opcode_table[0x22] = ldi_hl_a;    // LD (HL+),A
    opcode_table[0x2A] = ldi_a_hl;    // LD A,(HL+)
    opcode_table[0x32] = ldd_hl_a;    // LD (HL-),A
    opcode_table[0x3A] = ldd_a_hl;    // LD A,(HL-)

    // 16-BIT LOAD INSTRUCTIONS
    opcode_table[0x01] = ld_rr_nn;    // LD BC,nn
    opcode_table[0x11] = ld_rr_nn;    // LD DE,nn
    opcode_table[0x21] = ld_rr_nn;    // LD HL,nn
    opcode_table[0x31] = ld_rr_nn;    // LD SP,nn
    opcode_table[0xF9] = ld_sp_hl;    // LD SP,HL
    opcode_table[0xF8] = ld_hl_sp_d;  // LD HL,SP+d
    opcode_table[0x08] = ld_nn_sp;    // LD (nn),SP
    opcode_table[0xC5] = push_rr;     // PUSH BC
    opcode_table[0xD5] = push_rr;     // PUSH DE
    opcode_table[0xE5] = push_rr;     // PUSH HL
    opcode_table[0xF5] = push_rr;     // PUSH AF
    opcode_table[0xC1] = pop_rr;      // POP BC
    opcode_table[0xD1] = pop_rr;      // POP DE
    opcode_table[0xE1] = pop_rr;      // POP HL
    opcode_table[0xF1] = pop_rr;      // POP AF

    // 8-BIT ALU
    for (int i = 0x80; i <= 0x87; i++) opcode_table[i] = add_a_r;  // ADD A,r
    for (int i = 0x88; i <= 0x8F; i++) opcode_table[i] = adc_a_r;  // ADC A,r
    for (int i = 0x90; i <= 0x97; i++) opcode_table[i] = sub_a_r;  // SUB A,r
    for (int i = 0x98; i <= 0x9F; i++) opcode_table[i] = sbc_a_r;  // SBC A,r
    for (int i = 0xA0; i <= 0xA7; i++) opcode_table[i] = and_r;    // AND r
    for (int i = 0xA8; i <= 0xAF; i++) opcode_table[i] = xor_r;    // XOR r
    for (int i = 0xB0; i <= 0xB7; i++) opcode_table[i] = or_r;     // OR r
    for (int i = 0xB8; i <= 0xBF; i++) opcode_table[i] = cp_r;     // CP r
    opcode_table[0xC6] = add_a_n;     // ADD A,n
    opcode_table[0xCE] = adc_a_n;     // ADC A,n
    opcode_table[0xD6] = sub_a_n;     // SUB A,n
    opcode_table[0xDE] = sbc_a_n;     // SBC A,n
    opcode_table[0xE6] = and_n;       // AND n
    opcode_table[0xEE] = xor_n;       // XOR n
    opcode_table[0xF6] = or_n;        // OR n
    opcode_table[0xFE] = cp_n;        // CP n
    opcode_table[0x04] = inc_r;       // INC B
    opcode_table[0x0C] = inc_r;       // INC C
    opcode_table[0x14] = inc_r;       // INC D
    opcode_table[0x1C] = inc_r;       // INC E
    opcode_table[0x24] = inc_r;       // INC H
    opcode_table[0x2C] = inc_r;       // INC L
    opcode_table[0x34] = inc_r;       // INC (HL)
    opcode_table[0x3C] = inc_r;       // INC A
    opcode_table[0x05] = dec_r;       // DEC B
    opcode_table[0x0D] = dec_r;       // DEC C
    opcode_table[0x15] = dec_r;       // DEC D
    opcode_table[0x1D] = dec_r;       // DEC E
    opcode_table[0x25] = dec_r;       // DEC H
    opcode_table[0x2D] = dec_r;       // DEC L
    opcode_table[0x35] = dec_r;       // DEC (HL)
    opcode_table[0x3D] = dec_r;       // DEC A

    // 16-BIT ALU
    opcode_table[0x09] = add_hl_rr;   // ADD HL,BC
    opcode_table[0x19] = add_hl_rr;   // ADD HL,DE
    opcode_table[0x29] = add_hl_rr;   // ADD HL,HL
    opcode_table[0x39] = add_hl_rr;   // ADD HL,SP
    opcode_table[0xE8] = add_sp_d;    // ADD SP,d
    opcode_table[0x03] = inc_rr;      // INC BC
    opcode_table[0x13] = inc_rr;      // INC DE
    opcode_table[0x23] = inc_rr;      // INC HL
    opcode_table[0x33] = inc_rr;      // INC SP
    opcode_table[0x0B] = dec_rr;      // DEC BC
    opcode_table[0x1B] = dec_rr;      // DEC DE
    opcode_table[0x2B] = dec_rr;      // DEC HL
    opcode_table[0x3B] = dec_rr;      // DEC SP

    // ROTATES AND SHIFTS
    opcode_table[0x07] = rlca;        // RLCA
    opcode_table[0x17] = rla;         // RLA
    opcode_table[0x0F] = rrca;        // RRCA
    opcode_table[0x1F] = rra;         // RRA

    // CPU CONTROL
    opcode_table[0x00] = op_nop;      // NOP
    opcode_table[0x76] = halt;        // HALT
    opcode_table[0x10] = stop;        // STOP
    opcode_table[0xF3] = di;          // DI
    opcode_table[0xFB] = ei;          // EI
    opcode_table[0x37] = scf;         // SCF
    opcode_table[0x3F] = ccf;         // CCF

    // JUMPS AND CALLS
    opcode_table[0xC3] = jp_nn;       // JP nn
    opcode_table[0xC2] = jp_cc_nn;    // JP NZ,nn
    opcode_table[0xCA] = jp_cc_nn;    // JP Z,nn
    opcode_table[0xD2] = jp_cc_nn;    // JP NC,nn
    opcode_table[0xDA] = jp_cc_nn;    // JP C,nn
    opcode_table[0xE9] = jp_hl;       // JP (HL)
    opcode_table[0x18] = jr_n;        // JR n
    opcode_table[0x20] = jr_cc_n;     // JR NZ,n
    opcode_table[0x28] = jr_cc_n;     // JR Z,n
    opcode_table[0x30] = jr_cc_n;     // JR NC,n
    opcode_table[0x38] = jr_cc_n;     // JR C,n
    opcode_table[0xCD] = call_nn;     // CALL nn
    opcode_table[0xC4] = call_cc_nn;  // CALL NZ,nn
    opcode_table[0xCC] = call_cc_nn;  // CALL Z,nn
    opcode_table[0xD4] = call_cc_nn;  // CALL NC,nn
    opcode_table[0xDC] = call_cc_nn;  // CALL C,nn
    opcode_table[0xC9] = ret;         // RET
    opcode_table[0xC0] = ret_cc;      // RET NZ
    opcode_table[0xC8] = ret_cc;      // RET Z
    opcode_table[0xD0] = ret_cc;      // RET NC
    opcode_table[0xD8] = ret_cc;      // RET C
    opcode_table[0xD9] = reti;        // RETI
    opcode_table[0xC7] = rst_n;       // RST 00H
    opcode_table[0xCF] = rst_n;       // RST 08H
    opcode_table[0xD7] = rst_n;       // RST 10H
    opcode_table[0xDF] = rst_n;       // RST 18H
    opcode_table[0xE7] = rst_n;       // RST 20H
    opcode_table[0xEF] = rst_n;       // RST 28H
    opcode_table[0xF7] = rst_n;       // RST 30H
    opcode_table[0xFF] = rst_n;       // RST 38H

    // CB PREFIX OPERATIONS
    for (int i = 0x00; i <= 0x07; i++) cb_opcode_table[i] = rlc_r;        // RLC r
    for (int i = 0x08; i <= 0x0F; i++) cb_opcode_table[i] = rrc_r;        // RRC r
    for (int i = 0x10; i <= 0x17; i++) cb_opcode_table[i] = rl_r;         // RL r
    for (int i = 0x18; i <= 0x1F; i++) cb_opcode_table[i] = rr_r;         // RR r
    for (int i = 0x20; i <= 0x27; i++) cb_opcode_table[i] = sla_r;        // SLA r
    for (int i = 0x28; i <= 0x2F; i++) cb_opcode_table[i] = sra_r;        // SRA r
    for (int i = 0x30; i <= 0x37; i++) cb_opcode_table[i] = swap_r;       // SWAP r
    for (int i = 0x38; i <= 0x3F; i++) cb_opcode_table[i] = srl_r;        // SRL r
    for (int i = 0x40; i <= 0x7F; i++) cb_opcode_table[i] = bit_n_r;      // BIT n,r
    for (int i = 0x80; i <= 0xBF; i++) cb_opcode_table[i] = res_n_r;      // RES n,r
    for (int i = 0xC0; i <= 0xFF; i++) cb_opcode_table[i] = set_n_r;      // SET n,r
}

// CPU CYCLES
void cpu_init(void) {
    init_tables();

    CPU.reg.pc = 0x0000;
    CPU.reg.sp = 0xFFFE;
    
    // Default register values
    CPU.reg.a = 0x01;
    CPU.reg.f = 0xB0;
    CPU.reg.b = 0x00;
    CPU.reg.c = 0x13;
    CPU.reg.d = 0x00;
    CPU.reg.e = 0xD8;
    CPU.reg.h = 0x01;
    CPU.reg.l = 0x4D;

    CPU.ime = 1;
    CPU.ime_scheduled = 0;
    CPU.halted = false;
    CPU.stopped = false;

    CPU.opcode = 0;
    CPU.cycles = 0;
}

void cpu_step(void) {
    if (CPU.halted) return;

    // SET FLAG FOR IME SCHEDULER
    bool was_scheduled = CPU.ime_scheduled;

    // GET + STORE INITIAL PC FOR DEBUG
    u16 initial_pc = CPU.reg.pc;
    
    LOG_TRACE(LOG_CPU, "--- NEW CPU CYCLE ---");
    LOG_TRACE(LOG_CPU, "ATTEMPTING TO READ FROM PC: 0x%04X\n", CPU.reg.pc);

    // GET CURRENT OPCODE
    CPU.opcode = read_from_bus(CPU.reg.pc++);

    LOG_DEBUG(LOG_CPU, "READ COMPLETE - OPCODE: 0x%02X, NEW PC: 0x%04X\n", CPU.opcode, CPU.reg.pc);
    LOG_TRACE(LOG_CPU, "IME=%d IME_SCHEDULED=%d SP=0x%04X CYCLES=%u",
        CPU.ime, CPU.ime_scheduled, CPU.reg.sp, CPU.cycles);
    LOG_TRACE(LOG_CPU, "REGS: A=0x%02X F=0x%02X B=0x%02X C=0x%02X D=0x%02X E=0x%02X H=0x%02X L=0x%02X",
        CPU.reg.a, CPU.reg.f, CPU.reg.b, CPU.reg.c, CPU.reg.d, CPU.reg.e, CPU.reg.h, CPU.reg.l);
    LOG_TRACE(LOG_CPU, "EXECUTING OPCODE 0x%02X at PC 0x%04X\n", CPU.opcode, initial_pc);

    // EXECUTE NEXT INSTRUCTION
    if (CPU.opcode == 0xCB) {
        CPU.opcode = read_from_bus(CPU.reg.pc++);
        cb_opcode_table[CPU.opcode]();
    } else {
        opcode_table[CPU.opcode]();
    }
    LOG_TRACE(LOG_CPU, "EXECUTION COMPLETE - PC NOW AT: 0x%04X\n", CPU.reg.pc);

    if (was_scheduled) {
        LOG_TRACE(LOG_CPU, "ENABLING IME (WAS SCHEDULED FROM PREVIOUS INSTRUCTION)\n");
        CPU.ime = 1;
        CPU.ime_scheduled = 0;
    }

    LOG_TRACE(LOG_CPU, "\nAFTER OP:\n");  
    LOG_TRACE(LOG_CPU, "IME FLAG: %i, IME-SCHEDULED: %i\n", CPU.ime, CPU.ime_scheduled);
    LOG_TRACE(LOG_CPU, "PC: 0x%04X  SP: 0x%04X  Cycles: %u\n", 
           CPU.reg.pc, CPU.reg.sp, CPU.cycles);
    LOG_TRACE(LOG_CPU, "A: 0x%02X  F: 0x%02X\n", CPU.reg.a, CPU.reg.f);
    LOG_TRACE(LOG_CPU, "B: 0x%02X  C: 0x%02X\n", CPU.reg.b, CPU.reg.c);
    LOG_TRACE(LOG_CPU, "D: 0x%02X  E: 0x%02X\n", CPU.reg.d, CPU.reg.e);
    LOG_TRACE(LOG_CPU, "H: 0x%02X  L: 0x%02X\n", CPU.reg.h, CPU.reg.l);
    LOG_TRACE(LOG_CPU, "--- END CPU CYCLE ---");
}

// CYCLE MANAGEMENT
u8 get_cpu_cycles(void) {
    return CPU.cycles;
}

void reset_cpu_cycles(void) {
    CPU.cycles = 0;
}

// REGISTER ACCESS
registers* get_registers(void) {
    return &CPU.reg;
}

// INTERRUPT STATE HANDLING
bool get_ime(void) {
    return CPU.ime;
}

void set_ime(bool val) {
    CPU.ime = val;
}

bool is_cpu_halted(void) {
    return CPU.halted;
}

void set_cpu_halted(bool val) {
    CPU.halted = val;
}
