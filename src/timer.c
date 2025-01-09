#include "logger.h"
#include "timer.h"

static struct {
    u16 div;        // DIVIDER REG (INCR AT SYSTEM CLOCK)
    u8 tima;        // TIMER COUNTER (INCR AT FREQUENCY SET BY TAC)
    u8 tma;         // TIMER MODULO (LOADED INTO TIMA ON OVERFLOW)
    u8 tac;         // TIMER CONTROL
    bool prev_bit;  // EDGE DETECTION
} timer = {0};

static bool get_timer_bit(void) {
    // GET BIT BASED ON FREQUENCY SELECTION
    switch (timer.tac & TAC_FREQ_MASK) {
        case FREQ_4KHZ:   return (timer.div & (1 << 9));  // BIT 9
        case FREQ_262KHZ: return (timer.div & (1 << 3));  // BIT 3
        case FREQ_65KHZ:  return (timer.div & (1 << 5));  // BIT 5
        case FREQ_16KHZ:  return (timer.div & (1 << 7));  // BIT 7
        default: return false;
    }
}

void timer_init(void) {
    timer.div = 0x00;
    timer.tima = 0;
    timer.tma = 0;
    timer.tac = 0;
    timer.prev_bit = false;
    LOG_INFO(LOG_TIMER, "TIMER INITIALIZED AT DIV=0x%04X\n", timer.div);
}

void timer_tick(void) {
    // INCREMENT SYSTEM DIVIDER (ALWAYS RUNS)
    timer.div++;
    
    // CHECK IF TIMER IS ENABLED
    if (!(timer.tac & TAC_ENABLE)) {
        timer.prev_bit = get_timer_bit();
        return;
    }

    // GET CURRENT BIT FOR FREQUENCY
    bool current_bit = get_timer_bit();
    
    // FALLING EDGE DETECTION (1->0 TRANSITION)
    if (timer.prev_bit && !current_bit) {
        // CHECK FOR OVERFLOW
        if (++timer.tima == 0) {
            timer.tima = timer.tma;     // LOAD MODULO VALUE
            interrupt_req(INT_TIMER);   // REQUEST INTERRUPT
            LOG_WARN(LOG_TIMER, "TIMER OVERFLOW - LOADED TMA=0x%02X\n", timer.tma);
        }
    }
    
    timer.prev_bit = current_bit;
}

u8 timer_read(u16 addr) {
    switch(addr) {
        case DIV_REG:     // 0xFF04
            return timer.div >> 8;  // UPPER 8 BITS OF DIV
        case TIMA_REG:    // 0xFF05
            return timer.tima;
        case TMA_REG:     // 0xFF06
            return timer.tma;
        case TAC_REG:     // 0xFF07
            return timer.tac;
        default:
            LOG_WARN(LOG_TIMER, "INVALID TIMER READ: 0x%04X\n", addr);
            return 0xFF;
    }
}

void timer_write(u16 addr, u8 val) {
    switch(addr) {
        case DIV_REG:     // 0xFF04
            timer.div = 0;  // ANY WRITE RESETS DIV
            LOG_INFO(LOG_TIMER, "DIV RESET TO 0\n");
            break;
            
        case TIMA_REG:    // 0xFF05
            timer.tima = val;
            LOG_INFO(LOG_TIMER, "TIMA SET TO 0x%02X\n", val);
            break;
            
        case TMA_REG:     // 0xFF06
            timer.tma = val;
            LOG_INFO(LOG_TIMER, "TMA SET TO 0x%02X\n", val);
            break;
            
        case TAC_REG:     // 0xFF07
            // ONLY LOWER 3 BITS ARE WRITABLE
            timer.tac = val & 0x07;
            LOG_INFO(LOG_TIMER, "TAC SET TO 0x%02X (TIMER %s, FREQ=%d)\n", 
                   timer.tac,
                   (timer.tac & TAC_ENABLE) ? "ENABLED" : "DISABLED",
                   timer.tac & TAC_FREQ_MASK);
            break;
            
        default:
            LOG_ERROR(LOG_TIMER, "INVALID TIMER WRITE: 0x%04X = 0x%02X\n", addr, val);
            break;
    }
}
