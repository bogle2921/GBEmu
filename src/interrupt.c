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

void handle_interrupts() {
    if (!get_ime()) {
        return;
    }

    u8 active = interrupt.flags & interrupt.enable;
    if (!active) return;

    registers* reg = get_registers();

    // HANDLE ONE INTERRUPT AT A TIME IN PRIORITY ORDER
    // I DONT THINK WE ARE DOING THIS RIGHT, TOO AGGRESSIVE
    if (active & INT_VBLANK) {
        interrupt.flags &= ~INT_VBLANK;
        LOG_INFO(LOG_INTERRUPT, "!! -- VBLANK INTERRUPT -- !!\n");
        set_ime(false);
        stack_push16(reg->pc);
        reg->pc = 0x40;
        set_cpu_halted(false);
    }
    else if (active & INT_LCD) {
        interrupt.flags &= ~INT_LCD;
        LOG_INFO(LOG_INTERRUPT, "!! -- LCD INTERRUPT -- !!\n");
        set_ime(false);
        stack_push16(reg->pc);
        reg->pc = 0x48;
        set_cpu_halted(false);
    }
    else if (active & INT_TIMER) {
        interrupt.flags &= ~INT_TIMER;
        LOG_INFO(LOG_INTERRUPT, "!! -- TIMER INTERRUPT -- !!\n");
        set_ime(false);
        stack_push16(reg->pc);
        reg->pc = 0x50;
        set_cpu_halted(false);
    }
    else if (active & INT_SERIAL) {
        interrupt.flags &= ~INT_SERIAL;
        LOG_INFO(LOG_INTERRUPT, "!! -- SERIAL INTERRUPT -- !!\n");
        set_ime(false);
        stack_push16(reg->pc);
        reg->pc = 0x58;
        set_cpu_halted(false);
    }
    else if (active & INT_JOYPAD) {
        interrupt.flags &= ~INT_JOYPAD;
        LOG_INFO(LOG_INTERRUPT, "!! -- JOYPAD INTERRUPT -- !!\n");
        set_ime(false);
        stack_push16(reg->pc);
        reg->pc = 0x60;
        set_cpu_halted(false);
    }
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
