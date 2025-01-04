#include "interrupt.h"

void push_interrupt(cpu* CPU, u16 addr){
    stack_push16(CPU->reg.pc);
    CPU->reg.pc = addr;
    
}

bool check_interrupt(cpu* CPU, u16 addr, interrupts i){
    if(CPU->intrupt_flags & i && CPU->ie_reg & i){
        push_interrupt(CPU, addr);
        CPU->intrupt_flags &= ~i;
        CPU->halted = false;
        CPU->ime = false;
        return true;
    }
    return false;
}

void handle_interrupts(cpu* CPU){
    if(check_interrupt(CPU, 0x40, INT_VBLANK)){

    } else if (check_interrupt(CPU, 0x48, INT_LCD)){

    } else if(check_interrupt(CPU, 0x50, INT_TIMER)){

    } else if(check_interrupt(CPU, 0x58, INT_SERIAL)){

    } else if(check_interrupt(CPU, 0x60, INT_JOYPAD)){
        
    }
}