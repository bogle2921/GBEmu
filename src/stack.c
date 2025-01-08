#include "logger.h"
#include "stack.h"

void stack_push8(u8 val){
    get_registers()->sp--;
    write_to_bus(get_registers()->sp, val);
}

void stack_push16(u16 val){
    stack_push8((val >> 8) & 0xFF);
    stack_push8(val & 0xFF);
}

u8 stack_pop8(){
    return read_from_bus(get_registers()->sp++);
}

u16 stack_pop16(){
    u16 low = stack_pop8();
    u16 high = stack_pop8();

    return MAKE_WORD(high, low);
}