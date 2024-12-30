#include "cart.h"
#include "bus.h"


// VIRTUALIZED MEMORY (OUR RAM)
static u8 vram[0x2000];         // 8KB VRAM
static u8 wram[0x2000];         // 8KB WRAM
static u8 oam[0xA0];            // 160B OAM
static u8 hram[0x7F];           // 127B HRAM
static u8 ie_register;          // INTERRUPT ENABLE REGISTER, SEE CONFIG
static u8 io_registers[0x80];   // STORE IO REGISTER VALUES

u8 read_io(u16 addr) {
    switch(addr) {
        case P1_REG:
            // TODO: JOYPAD / CONTROLLER INP *READ*
            return io_registers[addr - IO_REG];

        case DIV_REG:
        case TIMA_REG:
        case TMA_REG:
        case TAC_REG:
            // TODO: TIMER WHICH IS GOING TO BE DIFFICULT I THINK *READ*
            return io_registers[addr - IO_REG];

        case IF_REG:
            // INTERRUPT FLAGS
            return io_registers[addr - IO_REG];

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
        case WX_REG:
            // TODO (YES YOU BANDIT): IMPLEMENT PPU
            return io_registers[addr - IO_REG];

        case NR10_REG:
        case NR11_REG:
        case NR12_REG:
        case NR13_REG:
        case NR14_REG:
        case NR21_REG:
        case NR22_REG:
        case NR23_REG:
        case NR24_REG:
        case NR30_REG:
        case NR31_REG:
        case NR32_REG:
        case NR33_REG:
        case NR34_REG:
        case NR41_REG:
        case NR42_REG:
        case NR43_REG:
        case NR44_REG:
        case NR50_REG:
        case NR51_REG:
        case NR52_REG:
            // TODO: IMPLEMENT SOUND
            return io_registers[addr - IO_REG];

        default:
            printf("BAD IO READ: 0x%04X\n", addr);
            return 0xFF;
    }
}

void write_io(u16 addr, u8 val) {
    switch(addr) {
        case P1_REG:
            // TODO: JOYPAD / CONTROLLER INP *WRITE*
            io_registers[addr - IO_REG] = val;
            break;

        case DIV_REG:
            // DIV IS RESET ON ANY WRITE
            io_registers[addr - IO_REG] = 0;
            break;

        case TIMA_REG:
        case TMA_REG:
        case TAC_REG:
            // TODO: IMPLEMENT TIMER
            io_registers[addr - IO_REG] = val;
            break;

        case IF_REG:
            // INTERRUPT FLAGS
            io_registers[addr - IO_REG] = val;
            break;

        case LCDC_REG:
        case STAT_REG:
        case SCY_REG:
        case SCX_REG:
        case LYC_REG:
        case DMA_REG:
        case BGP_REG:
        case OBP0_REG:
        case OBP1_REG:
        case WY_REG:
        case WX_REG:
            // TODO (YES YOU BANDIT): IMPLEMENT PPU
            io_registers[addr - IO_REG] = val;
            break;

        case LY_REG:
            // LY IS READ-ONLY
            break;

        case NR10_REG:
        case NR11_REG:
        case NR12_REG:
        case NR13_REG:
        case NR14_REG:
        case NR21_REG:
        case NR22_REG:
        case NR23_REG:
        case NR24_REG:
        case NR30_REG:
        case NR31_REG:
        case NR32_REG:
        case NR33_REG:
        case NR34_REG:
        case NR41_REG:
        case NR42_REG:
        case NR43_REG:
        case NR44_REG:
        case NR50_REG:
        case NR51_REG:
        case NR52_REG:
            // TODO: IMPLEMENT SOUND
            io_registers[addr - IO_REG] = val;
            break;

        default:
            printf("BAD IO WRITE: 0x%04X = 0x%02X\n", addr, val);
            break;
    }
}

u8 read_from_bus(u16 addr) {
    // ROM BANKS (0x0000 - 0x7FFF)
    if (addr < VRAM_START) {
        return read_cart(addr);
    }
    
    // VRAM (0x8000 - 0x9FFF)
    if (addr < RAM_START) {
        return vram[addr - VRAM_START];
    }
    
    // EXTERNAL RAM (0xA000 - 0xBFFF)
    if (addr < WRAM_START) {
        return read_cart_ram(addr - RAM_START);
    }
    
    // WRAM (0xC000 - 0xDFFF)
    if (addr < ECHO_RAM) {
        return wram[addr - WRAM_START];
    }
    
    // ECHO RAM (0xE000 - 0xFDFF)
    if (addr < OAM_START) {
        return wram[addr - ECHO_RAM];  // MIRRORS WRAM
    }
    
    // OAM (0xFE00 - 0xFE9F)
    if (addr < PROHIB_START) {
        return oam[addr - OAM_START];
    }
    
    // PROHIBITED (0xFEA0 - 0xFEFF)
    if (addr < HMEM_START) {
        return 0xFF;  // USUALLY RETURNS ALL 1S
    }
    
    // IO REGISTERS (0xFF00 - 0xFF7F)
    if (addr < HRAM) {
        return read_io(addr);
    }
    
    // HRAM (0xFF80 - 0xFFFE)
    if (addr < IE_REG) {
        return hram[addr - HRAM];
    }
    
    // IE REGISTER (0xFFFF)
    return ie_register;
}

void write_to_bus(u16 addr, u8 val) {
    // ROM BANKS (0x0000 - 0x7FFF)
    if (addr < VRAM_START) {
        write_to_cart(addr, val);  // HANDLE BANK SWITCHING
        return;
    }
    
    // VRAM (0x8000 - 0x9FFF)
    if (addr < RAM_START) {
        vram[addr - VRAM_START] = val;
        return;
    }
    
    // EXTERNAL RAM (0xA000 - 0xBFFF)
    if (addr < WRAM_START) {
        write_cart_ram(addr - RAM_START, val);
        return;
    }
    
    // WRAM (0xC000 - 0xDFFF)
    if (addr < ECHO_RAM) {
        wram[addr - WRAM_START] = val;
        return;
    }
    
    // ECHO RAM (0xE000 - 0xFDFF)
    if (addr < OAM_START) {
        wram[addr - ECHO_RAM] = val;  // MIRRORS WRAM
        return;
    }
    
    // OAM (0xFE00 - 0xFE9F)
    if (addr < PROHIB_START) {
        oam[addr - OAM_START] = val;
        return;
    }
    
    // PROHIBITED (0xFEA0 - 0xFEFF)
    if (addr < HMEM_START) {
        return;  // WRITES ARE IGNORED
    }
    
    // IO REGISTERS (0xFF00 - 0xFF7F)
    if (addr < HRAM) {
        write_io(addr, val);
        return;
    }
    
    // HRAM (0xFF80 - 0xFFFE)
    if (addr < IE_REG) {
        hram[addr - HRAM] = val;
        return;
    }
    
    // IE REGISTER (0xFFFF)
    ie_register = val;
}
