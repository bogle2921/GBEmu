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

// -- OPCODE FN POINTER TABLE --
// TODO: OBVIOUSLY FINISH IMPLEMENTING OP CODES, THERE ARE A LOT OF THEM

// INIT TABLE W/ DEFAULT FN
static opcode_func opcode_table[256] = {
    [0x00 ... 0xFF] = op_unknown // DIDNT KNOW RANGE EXISTED IN C, FIRST TIME USING THIS...
};

// SET VALID OPCODES
static void init_opcode_table(void) {
    opcode_table[0x00] = op_nop;
    opcode_table[0xC3] = op_jp_nz;
    opcode_table[0xFE] = op_cp_d8;
    opcode_table[0x76] = op_halt;
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
