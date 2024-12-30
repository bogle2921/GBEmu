#include "instructions.h"
#include "cpu.h"
#include "bus.h"

// STATIC GLOBAL CPU INSTANCE
static struct cpu CPU = {0};

// REGISTER PAIR HELPERS FOR COMMON MOV OPERATIONS
#define BC ((CPU.registers.b << 8) | CPU.registers.c)
#define DE ((CPU.registers.d << 8) | CPU.registers.e)
#define HL ((CPU.registers.h << 8) | CPU.registers.l)
#define AF ((CPU.registers.a << 8) | CPU.registers.f)

// -- FLAG OPERATIONS --
static inline void set_flags(u8 z, u8 n, u8 h, u8 c) {
    CPU.registers.f = (z ? FLAG_Z : 0) |    // Bit 7: Z (Zero flag)      - Set if result is zero
                      (n ? FLAG_N : 0) |    // Bit 6: N (Subtract flag)  - Set if last op was subtraction
                      (h ? FLAG_H : 0) |    // Bit 5: H (Half carry)     - Set if carry from bit 3 to 4
                      (c ? FLAG_C : 0);     // Bit 4: C (Carry flag)     - Set if result overflowed
                                            // Bits 3-0: Always zero
}

static inline bool get_flag(u8 flag) {
    return CPU.registers.f & flag;
}

// -- MEMORY OPERATIONS --
static inline u8 read_byte() {
    return read_from_bus(CPU.registers.pc++);
}

static inline u16 read_word() {
    return read_from_bus(CPU.registers.pc++) |
          (read_from_bus(CPU.registers.pc++) << 8);
}


//                                 -- OPCODES --

typedef void (*opcode_func)(void);  // DEFAULT
static void op_unknown(void) {
    printf("UNKNOWN OPCODE: 0x%02X\n", CPU.opcode);
    CPU.isHalted = true;
}

static void op_nop(void) {          // NOP
}

static void op_jp_nz(void) {        // JP NZ
    u16 jump_addr = read_word();
    if (!get_flag(FLAG_Z)) {
        CPU.registers.pc = jump_addr;
    }
}

static void op_cp_d8(void) {        // CP d8
    u8 value = read_byte();
    set_flags(
        CPU.registers.a - value == 0,
        1,
        (CPU.registers.a & 0xF) < (value & 0xF),
        CPU.registers.a < value
    );
}

static void op_halt(void) {         // HALT
    CPU.isHalted = true;
}

static void op_jr_z(void) {         // JR Z, r8
    int8_t offset = (int8_t)read_byte();
    if (get_flag(FLAG_Z)) {
        CPU.registers.pc += offset;
    }
}

static void op_xor_a(void) {        // XOR A
    // XOR OPS USE A REG
    // A XOR A IS ALWAYS 0 - does this really just clear out the A register?
    //                     - all XOR opcodes and OR opcodes are written in this way
    //                     - A is never XOR'd against a value?
    CPU.registers.a = 0;

    // SINCE 0, WE SET ZERO FLAG
    set_flags(1, 0, 0, 0);
}

static void op_jr(void) {           // JR r8
    int8_t offset = (int8_t)read_byte();
    CPU.registers.pc += offset;
}

static void op_ld_a16_a(void) { // LD (a16),A
    CPU.memory_destination = read_word();
    CPU.data_fetch = CPU.registers.a;
}

static void op_jr_nz_r8(void) { // JR NZ r8
    int8_t offset = (int8_t)read_byte();
    if(! get_flag(FLAG_Z)){
        CPU.registers.pc += offset;
    }
}

// H in LDH refers to HRAM - does this matter in this context?
// HRAM is Reg C?
static void op_ldh_a8_a(void) { // LDH (a8),A
    CPU.data_fetch = CPU.registers.a;
    CPU.memory_destination = CPU.registers.a | 0xFF00;
    CPU.registers.pc++;
}

static void op_ldh_a_a8(void) { // LDH A,(a8)
    CPU.registers.a = read_byte();
    CPU.memory_destination = CPU.registers.a | 0xFF00;
}

static void op_ld_a_d8(void) { // LD A,d8
    CPU.registers.a = read_byte();
}

static void op_ld_c_d8(void) { // LD C,d8
    CPU.registers.c = read_byte();
}

static void op_cpl(void) { // CPL - https://learn.cemetech.net/index.php/Z80:Opcodes:CPL
    CPU.registers.a ^= 0xFF;
    set_flags(0, 1, 1, 0);
}

static void op_and_d8(void) { // AND d8
    u8 val = read_byte();
    set_flags(CPU.registers.a & val == 0, 0, 1, 0);
}

// CB - another table
// fortunately the table cycles
reg_name cb_reg_lu[] = {
    REG_B, REG_C, REG_D, REG_E, REG_H, REG_L, REG_HL, REG_A
};

reg_name get_register(u8 reg_index){
    return cb_reg_lu[reg_index];
}

u8 read_register(reg_name reg){
    switch(reg){
        case REG_A: return CPU.registers.a;
        case REG_B: return CPU.registers.b;
        case REG_C: return CPU.registers.c;
        case REG_D: return CPU.registers.d;
        case REG_E: return CPU.registers.e;
        case REG_H: return CPU.registers.h;
        case REG_L: return CPU.registers.l;
        // operates on (HL) which implies we need to read_byte again. All bits are 0-7 which would be L
        // Hoping I have time to think on this before it needs implementation
        /*case REG_HL: {
            
        } */
        default:
            printf("unknown register- %d\n", reg);
            exit(-1);
    }
}

void set_register(reg_name reg, u8 val){
    switch(reg){
        case REG_A: 
            CPU.registers.a = val & 0xFF;
            break;
        case REG_B:
            CPU.registers.b = val & 0xFF;
            break;
        case REG_C:
            CPU.registers.c = val & 0xFF;
            break;
        case REG_D:
            CPU.registers.d = val & 0xFF;
            break;
        case REG_E:
            CPU.registers.e = val & 0xFF;
            break;
        case REG_H:
            CPU.registers.h = val & 0xFF;
            break;
        case REG_L:
            CPU.registers.l = val & 0xFF;
            break;
        // operates on (HL) which implies we need to read_byte again. All bits are 0-7 which would be L
        // Hoping I have time to think on this before it needs implementation
        /*case REG_HL: {
            
        } */
        default:
            printf("unknown register- %d\n", reg);
            exit(-1);
    }
}
static void op_cb(void){
    u8 value = read_byte();
    reg_name reg = get_register(value & 0x8);
    u8 bit = (value >> 3) & 0x8;
    u8 operation = (value >> 6) & 0x8;
    u8 reg_val = read_register(reg);

    // handles cases BIT RES SET
    switch(operation){
        case 1:
            // BIT
            set_flags(!(reg_val & (1 << bit)), 0, 1, reg_val & (1 << bit));
            return;
        case 2:
            // RES
            reg_val &= ~(1 << bit);
            set_register(reg, reg_val);
            return;
        case 3:
            // Set
            reg_val |= (1 << bit);
            set_register(reg, reg_val);
            return;
        
        switch(bit){
            case 0:
                // RLC
                return;
            case 1:
                // RRC
                return;
            case 2:
                // RL
                return;
            case 3:
                // RR
                return;
            case 4:
                // SLA
                return;
            case 5:
                // SRA
                return;
            case 6:
                // SWAP
                return;
            case 7:
                // SRL
                return;
        }
    }
}
// -- OPCODE FN POINTER TABLE --
// TODO: OBVIOUSLY FINISH IMPLEMENTING OP CODES, THERE ARE A LOT OF THEM

// INIT TABLE W/ DEFAULT FN
static opcode_func opcode_table[256] = {
    [0x00 ... 0xFF] = op_unknown // DIDNT KNOW RANGE EXISTED IN C, FIRST TIME USING THIS...
};

// SET VALID OPCODES
// d8 / d16 - immediate 8/16 bits of data
// a8 - 8 bit unsigned data
// a16 - 16 bit address
// r8 - 8 bit signed data
static void init_opcode_table(void) {
    opcode_table[0x00] = op_nop;
    opcode_table[0xC3] = op_jp_nz;
    opcode_table[0xFE] = op_cp_d8;
    opcode_table[0x76] = op_halt;
    opcode_table[0x28] = op_jr_z;
    opcode_table[0xAF] = op_xor_a;
    opcode_table[0x18] = op_jr;
    opcode_table[0xEA] = op_ld_a16_a;
    opcode_table[0x20] = op_jr_nz_r8;
    opcode_table[0xE0] = op_ldh_a8_a;
    opcode_table[0xF0] = op_ldh_a_a8;
    opcode_table[0x3E] = op_ld_a_d8;
    opcode_table[0x0E] = op_ld_c_d8;
    opcode_table[0x2F] = op_cpl; 
    opcode_table[0xE6] = op_and_d8;
    opcode_table[0xCB] = op_cb;
}

void cpu_step() {
    if (CPU.isHalted) {
        return;
    }

    CPU.opcode = read_byte();
    opcode_table[CPU.opcode]();
}

void cpu_init() {
    init_opcode_table();
    CPU.registers.pc = 0x100;
    CPU.registers.sp = 0xFFFE;
    CPU.isHalted = false;
    CPU.isStepping = false;
}
