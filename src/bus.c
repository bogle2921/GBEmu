#include "logger.h"
#include "bus.h"

// MEMORY MAP
/*
THIS FILE IS A BIT MUCH. TODO: IDK
CHECK BUS.H HEADER FILE, LET KEEP ALL INDEX/LOCATIONS THERE.
*/
static struct {
    u8 wram[0x2000];    // WORKING RAM (8KB)
    u8 hram[0x80];      // HIGH RAM (127B)
    u8 io[0x80];        // IO REGISTERS (127B) 
    u8 bootrom[0x100];  // BOOT ROM (256B)
    
    // IO PORTS
    struct {
        u8 joypad;
        u8 serial_data;
        u8 serial_control;
        u8 nr10, nr11, nr12, nr13, nr14;  // CHANNEL 1  
        u8 nr21, nr22, nr23, nr24;        // CHANNEL 2
        u8 nr30, nr31, nr32, nr33, nr34;  // CHANNEL 3 
        u8 nr41, nr42, nr43, nr44;        // CHANNEL 4
        u8 nr50, nr51, nr52;              // CONTROL
        u8 wave_ram[16];                  // WAVE PATTERN RAM
    } ports;
} bus = {0};

void init_bus() {
    memset(&bus.wram, 0, sizeof(bus.wram));
    memset(&bus.hram, 0, sizeof(bus.hram));
    memset(&bus.io, 0, sizeof(bus.io));
    memset(&bus.ports, 0, sizeof(bus.ports));
}

static u8 read_io(u16 addr) {
    // HANDLE ALL IO PORT READS
    switch(addr) {
        // JOYPAD
        case P1_REG:   return bus.ports.joypad;
        case SB_REG:   return bus.ports.serial_data;
        case SC_REG:   return bus.ports.serial_control;

        // TIMER
        case DIV_REG:
        case TIMA_REG:
        case TMA_REG:
        case TAC_REG:  return timer_read(addr);

        // INTERRUPTS  
        case IF_REG:   return get_interrupt_flags();

        // SOUND CH1
        case NR10_REG: return bus.ports.nr10;
        case NR11_REG: return bus.ports.nr11; 
        case NR12_REG: return bus.ports.nr12;
        case NR13_REG: return bus.ports.nr13;
        case NR14_REG: return bus.ports.nr14;

        // SOUND CH2
        case NR21_REG: return bus.ports.nr21;
        case NR22_REG: return bus.ports.nr22;
        case NR23_REG: return bus.ports.nr23;
        case NR24_REG: return bus.ports.nr24;

        // SOUND CH3
        case NR30_REG: return bus.ports.nr30;
        case NR31_REG: return bus.ports.nr31;
        case NR32_REG: return bus.ports.nr32;
        case NR33_REG: return bus.ports.nr33;
        case NR34_REG: return bus.ports.nr34;

        // SOUND CH4
        case NR41_REG: return bus.ports.nr41;
        case NR42_REG: return bus.ports.nr42;
        case NR43_REG: return bus.ports.nr43;
        case NR44_REG: return bus.ports.nr44;

        // SOUND CTRL
        case NR50_REG: return bus.ports.nr50;
        case NR51_REG: return bus.ports.nr51;
        case NR52_REG: return bus.ports.nr52;

        // WAVE PATTERN RAM
        case WAVE_RAM_START ... WAVE_RAM_START + 0xF:
            return bus.ports.wave_ram[addr - WAVE_RAM_START];

        // LCD/PPU
        case LCDC_REG:
        case STAT_REG:
        case SCY_REG:
        case SCX_REG:
        case LY_REG:
        case LYC_REG:
        case DMA_REG:
        case BGP_REG:
        case OBP0_REG:
        case OBP1_REG:
        case WY_REG:
        case WX_REG:   return lcd_read(addr);

        default:
            LOG_WARN(LOG_BUS, "BAD IO READ: 0x%04X\n", addr);
            return 0xFF;
    }
}

static void write_io(u16 addr, u8 val) {
    // HANDLE ALL IO PORT WRITES
    switch(addr) {
        // BOOTROM CONTROL
        case BROM_UMAP:
            if (val) {
                set_bootrom_enable(false);
                LOG_WARN(LOG_BUS, "BOOTROM DISABLED + INTERRUPTS ENABLED\n");
            }
            break;

        // JOYPAD
        case P1_REG:   bus.ports.joypad = val; break;
        case SB_REG:   bus.ports.serial_data = val; break;
        case SC_REG:   bus.ports.serial_control = val; break;

        // TIMER
        case DIV_REG:
        case TIMA_REG:
        case TMA_REG:
        case TAC_REG:  timer_write(addr, val); break;

        // INTERRUPTS
        case IF_REG:   set_interrupt_flags(val); break;

        // SOUND CH1
        case NR10_REG: bus.ports.nr10 = val; break;
        case NR11_REG: bus.ports.nr11 = val; break;
        case NR12_REG: bus.ports.nr12 = val; break;
        case NR13_REG: bus.ports.nr13 = val; break;
        case NR14_REG: bus.ports.nr14 = val; break;

        // SOUND CH2
        case NR21_REG: bus.ports.nr21 = val; break;
        case NR22_REG: bus.ports.nr22 = val; break;
        case NR23_REG: bus.ports.nr23 = val; break;
        case NR24_REG: bus.ports.nr24 = val; break;

        // SOUND CH3  
        case NR30_REG: bus.ports.nr30 = val; break;
        case NR31_REG: bus.ports.nr31 = val; break;
        case NR32_REG: bus.ports.nr32 = val; break;
        case NR33_REG: bus.ports.nr33 = val; break;
        case NR34_REG: bus.ports.nr34 = val; break;

        // SOUND CH4
        case NR41_REG: bus.ports.nr41 = val; break;
        case NR42_REG: bus.ports.nr42 = val; break;
        case NR43_REG: bus.ports.nr43 = val; break;
        case NR44_REG: bus.ports.nr44 = val; break;

        // SOUND CTRL
        case NR50_REG: bus.ports.nr50 = val; break;
        case NR51_REG: bus.ports.nr51 = val; break;
        case NR52_REG: bus.ports.nr52 = val; break;

        // WAVE PATTERN RAM
        case WAVE_RAM_START ... WAVE_RAM_START + 0xF:
            bus.ports.wave_ram[addr - WAVE_RAM_START] = val;
            break;

        // LCD/PPU
        case LCDC_REG:
        case STAT_REG:
        case SCY_REG:
        case SCX_REG:
        case LY_REG:
        case LYC_REG:
        case DMA_REG:
        case BGP_REG:
        case OBP0_REG:
        case OBP1_REG:
        case WY_REG:
        case WX_REG:   lcd_write(addr, val); break;

        default:
            LOG_WARN(LOG_BUS, "BAD IO WRITE: 0x%04X = 0x%02X\n", addr, val);
            break;
    }
}

u8 read_from_bus(u16 addr) {
    // BOOTROM OVERRIDE WHEN ENABLED
    if (addr < 0x100) {
        if (get_bootrom_enable()) {
            return bus.bootrom[addr];
        }
        return read_cart(addr);  // FALLBACK TO CART
    }

    // MAIN MEMORY MAP
    if (addr < VRAM_START)     return read_cart(addr);
    if (addr < RAM_START)      return vram_read(addr);
    if (addr < WRAM_START)     return read_cart_ram(addr);
    if (addr < ECHO_RAM)       return bus.wram[addr - WRAM_START];
    if (addr < OAM_START)      return bus.wram[addr - ECHO_RAM];
    if (addr < PROHIB_START)   return get_dma_active() ? 0xFF : oam_read(addr);
    if (addr < IO_START)     return 0xFF;  // PROHIBITED
    if (addr < HRAM)           return read_io(addr);
    if (addr < IE_REG)         return bus.hram[addr - HRAM];
    
    return get_interrupt_enable();
}

void write_to_bus(u16 addr, u8 val) {
    // MAIN MEMORY MAP
    if (addr < VRAM_START)     write_to_cart(addr, val);
    else if (addr < RAM_START) vram_write(addr, val);
    else if (addr >= RAM_START && addr < WRAM_START) write_cart_ram(addr, val);
    else if (addr < ECHO_RAM)  bus.wram[addr - WRAM_START] = val;
    else if (addr < OAM_START) bus.wram[addr - ECHO_RAM] = val;
    else if (addr < PROHIB_START) {
        if (!get_dma_active()) oam_write(addr, val);
    }
    else if (addr < IO_START) return;  // PROHIBITED
    else if (addr < HRAM)     write_io(addr, val);
    else if (addr < IE_REG)   bus.hram[addr - HRAM] = val;
    else set_interrupt_enable(val);
}

void load_bootrom_data(u8* data, u32 size) {
    if (size > 0x100) size = 0x100;
    memcpy(bus.bootrom, data, size);
}

u8* get_memory_ptr(u16 addr) {
    // ONLY ALLOW DIRECT ACCESS TO WORK/HIGH RAM
    if (addr >= WRAM_START && addr < ECHO_RAM) {
        return &bus.wram[addr - WRAM_START];
    }
    if (addr >= HRAM && addr < IE_REG) {
        return &bus.hram[addr - HRAM];
    }
    return NULL;
}

void debug_dump_bootrom(u8 num_bytes) {
    LOG_DEBUG(LOG_BUS, "BOOTROM DUMP (%d BYTES): ", num_bytes);
    for (u8 i = 0; i < num_bytes; i++) {
        LOG_DEBUG(LOG_BUS, "%02X ", bus.bootrom[i]);
    }
    LOG_DEBUG(LOG_BUS, "\n");
}
