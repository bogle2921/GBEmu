#include "logger.h"
#include "interrupt.h"
#include "stack.h"

static struct {
    u8 flags;    // IF REGISTER
    u8 enable;   // IE REGISTER
} interrupt = {0};

void interrupt_init() {
    interrupt.flags = 0xE1;  // BOOT VALUE
    interrupt.enable = 0x00; // START WITH ALL DISABLED
    LOG_INFO(LOG_INTERRUPT, "INTERRUPTS INITIALIZED (IF=0x%02X, IE=0x%02X)\n", 
             interrupt.flags, interrupt.enable);
}

void interrupt_req(interrupts i) {
    interrupt.flags |= i;
}

u8 handle_interrupts() {
    u8 active = interrupt.flags & interrupt.enable;
    if (!active) return 0;

    // ANY PENDING INTERRUPT WAKES HALT REGARDLESS OF IME
    set_cpu_halted(false);

    // BUT THE HANDLER ONLY DISPATCHES WHEN IME IS SET
    if (!get_ime()) return 0;

    registers* reg = get_registers();
    u16 vector = 0;
    u8 which = 0;

    // PRIORITY ORDER: VBLANK > LCD > TIMER > SERIAL > JOYPAD
    if      (active & INT_VBLANK) { which = INT_VBLANK; vector = 0x40; }
    else if (active & INT_LCD)    { which = INT_LCD;    vector = 0x48; }
    else if (active & INT_TIMER)  { which = INT_TIMER;  vector = 0x50; }
    else if (active & INT_SERIAL) { which = INT_SERIAL; vector = 0x58; }
    else if (active & INT_JOYPAD) { which = INT_JOYPAD; vector = 0x60; }
    else return 0;

    interrupt.flags &= ~which;
    set_ime(false);
    stack_push16(reg->pc);
    reg->pc = vector;

    // ISR DISPATCH IS 5 M-CYCLES = 20 T-CYCLES
    return 20;
}

u8 get_interrupt_flags() {
    return interrupt.flags;
}

void set_interrupt_flags(u8 flags) {
    interrupt.flags = flags;
}

u8 get_interrupt_enable() {
    return interrupt.enable;
}

void set_interrupt_enable(u8 enable) {
    interrupt.enable = enable;
}
