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
    if (get_bootrom_enable()) {
        timer.div = 0x00;    // START AT 0 WITH BOOTROM
    } else {
        timer.div = 0xABCC;  // POST-BOOT VALUE
    }
    timer.tima = 0x00;
    timer.tma = 0x00;
    timer.tac = 0x00;
    timer.prev_bit = false;
    LOG_INFO(LOG_TIMER, "TIMER INITIALIZED AT DIV=0x%04X\n", timer.div);
}

void timer_tick(void) {
    // CALLED ONCE PER M-CYCLE (4 T-CYCLES). THE INTERNAL DIV COUNTER
    // ADVANCES EVERY T-CYCLE SO WE LOOP 4X TO CATCH ALL FALLING EDGES.
    for (int t = 0; t < 4; t++) {
        timer.div++;

        bool current_bit = get_timer_bit() && (timer.tac & TAC_ENABLE);

        // FALLING EDGE DETECTION (1->0 TRANSITION)
        if (timer.prev_bit && !current_bit) {
            if (++timer.tima == 0) {
                timer.tima = timer.tma;     // LOAD MODULO VALUE
                interrupt_req(INT_TIMER);   // REQUEST INTERRUPT
            }
        }
        timer.prev_bit = current_bit;
    }
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
            // WRITING DIV CAN CAUSE A TIMA TICK IF THE SELECTED BIT WAS HIGH
            // (FALLING EDGE FROM 1 TO 0 AS THE COUNTER RESETS).
            if (timer.prev_bit) {
                if (++timer.tima == 0) {
                    timer.tima = timer.tma;
                    interrupt_req(INT_TIMER);
                }
            }
            timer.div = 0;
            timer.prev_bit = false;
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
