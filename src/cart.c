#include "logger.h"
#include "cart.h"

// ZERO FOR SAFETY
struct cartridge c = {0};

// LICENSE BIT VALIDATION - THIS SHOULD BE IN HEADER. I DONT EVEN THINK WE USE IT ANYMORE
static const char* LICENSES[0xA5] = {
    [0x00] = "None",
    [0x01] = "Nintendo R&D1",
    [0x08] = "Capcom",
    [0x13] = "Electronic Arts",
    [0x18] = "Hudson Soft",
    [0x19] = "b-ai",
    [0x20] = "kss",
    [0x22] = "pow",
    [0x24] = "PCM Complete",
    [0x25] = "san-x",
    [0x28] = "Kemco Japan",
    [0x29] = "seta",
    [0x30] = "Viacom",
    [0x31] = "Nintendo",
    [0x32] = "Bandai",
    [0x33] = "Ocean/Acclaim",
    [0x34] = "Konami",
    [0x35] = "Hector",
    [0x37] = "Taito",
    [0x38] = "Hudson",
    [0x39] = "Banpresto",
    [0x41] = "Ubi Soft",
    [0x42] = "Atlus",
    [0x44] = "Malibu",
    [0x46] = "angel",
    [0x47] = "Bullet-Proof",
    [0x49] = "irem",
    [0x50] = "Absolute",
    [0x51] = "Acclaim",
    [0x52] = "Activision",
    [0x53] = "American sammy",
    [0x54] = "Konami",
    [0x55] = "Hi tech entertainment",
    [0x56] = "LJN",
    [0x57] = "Matchbox",
    [0x58] = "Mattel",
    [0x59] = "Milton Bradley",
    [0x60] = "Titus",
    [0x61] = "Virgin",
    [0x64] = "LucasArts",
    [0x67] = "Ocean",
    [0x69] = "Electronic Arts",
    [0x70] = "Infogrames",
    [0x71] = "Interplay",
    [0x72] = "Broderbund",
    [0x73] = "sculptured",
    [0x75] = "sci",
    [0x78] = "THQ",
    [0x79] = "Accolade",
    [0x80] = "misawa",
    [0x83] = "lozc",
    [0x86] = "Tokuma Shoten Intermedia",
    [0x87] = "Tsukuda Original",
    [0x91] = "Chunsoft",
    [0x92] = "Video system",
    [0x93] = "Ocean/Acclaim",
    [0x95] = "Varie",
    [0x96] = "Yonezawa/s'pal",
    [0x97] = "Kaneko",
    [0x98] = "RESERVED_FOR_LINUS",
    [0x99] = "Pack in soft",
    [0xA4] = "Konami (Yu-Gi-Oh!)"
};

bool load_bootrom(const char* bootrom) {
    LOG_INFO(LOG_MAIN, "LOADING BOOTROM: %s", bootrom);

    memset(c.boot_filename, 0, 1024);
    snprintf(c.boot_filename, sizeof(c.boot_filename), "%s", bootrom);
    FILE* boot_rom_file = fopen(bootrom, "rb");
    if (!boot_rom_file) {
        LOG_ERROR(LOG_MAIN, "FAILED TO OPEN BOOTROM: %s", bootrom);
        return false;
    }

    fseek(boot_rom_file, 0, SEEK_END);
    long file_size = ftell(boot_rom_file);
    if (file_size != 256) {
        LOG_ERROR(LOG_MAIN, "INVALID BOOTROM SIZE: %ld BYTES", file_size);
        fclose(boot_rom_file);
        return false;
    }
    rewind(boot_rom_file);
    c.boot_rom_size = (u32)file_size;

    // READ BOOTROM DATA FIRST
    u8 boot_data[256];
    size_t bytes_read = fread(boot_data, 1, c.boot_rom_size, boot_rom_file);
    if (bytes_read != c.boot_rom_size) {
        LOG_ERROR(LOG_MAIN, "FAILED TO READ ENTIRE BOOTROM");
        fclose(boot_rom_file);
        return false;
    }

    // COPY DIRECTLY TO BUS BOOTROM BUFFER 
    load_bootrom_data(boot_data, c.boot_rom_size);
    set_bootrom_enable(true);

    LOG_INFO(LOG_MAIN, "BOOTROM LOADED SUCCESSFULLY");
    fclose(boot_rom_file);
    debug_dump_bootrom(8);
    return true;
}

void set_bootrom_enable(bool enable) {
    extern bool use_bootrom;  // REF THE GLOBAL
    use_bootrom = enable;
}

bool get_bootrom_enable() {
    extern bool use_bootrom;  // REF THE GLOBAL
    return use_bootrom;
}

static bool is_mbc1() {
    return (c.header->type >= 1 && c.header->type <= 3);
}

static bool is_mbc2() {
    return (c.header->type == 0x05 || c.header->type == 0x06);
}

static bool is_mbc3() {
    return (c.header->type >= 0x0F && c.header->type <= 0x13);
}

static bool is_mbc5() {
    return (c.header->type >= 0x19 && c.header->type <= 0x1E);
}

static u32 get_rom_offset(u16 addr) {
    if (addr < ROM_BANK) return addr;  // BANK 0 FIXED

    if (is_mbc1()) {
        return (c.current_rom_bank * ROM_BANK) + (addr - ROM_BANK);
    }
    
    if (is_mbc3()) {
        return (c.current_rom_bank * ROM_BANK) + (addr - ROM_BANK);
    }

    // FALLBACK TO NO BANKING
    return addr;
}

static u32 get_ram_offset(u16 addr) {
    if (is_mbc1() || is_mbc3()) {
        return (c.current_ram_bank * RAM_BANK_SIZE) + (addr - RAM_START);
    }
    return addr - RAM_START;
}

bool load_cartridge(const char* cart) {
    LOG_INFO(LOG_MAIN, "LOADING CARTRIDGE: %s", cart);

    snprintf(c.filename, sizeof(c.filename), "%s", cart);
    FILE* rom_file = fopen(cart, "rb");
    if (!rom_file) {
        LOG_ERROR(LOG_MAIN, "FAILED TO OPEN ROM: %s", cart);
        return false;
    }

    fseek(rom_file, 0, SEEK_END);
    long file_size = ftell(rom_file);
    if (file_size <= 0) {
        LOG_ERROR(LOG_MAIN, "INVALID ROM SIZE: %ld BYTES", file_size);
        fclose(rom_file);
        return false;
    }
    rewind(rom_file);
    c.rom_size = (u32)file_size;

    c.rom_data = malloc(c.rom_size);
    if (!c.rom_data) {
        LOG_ERROR(LOG_MAIN, "FAILED TO ALLOCATE ROM MEMORY: %u BYTES", c.rom_size);
        fclose(rom_file);
        return false;
    }

    size_t bytes_read = fread(c.rom_data, 1, c.rom_size, rom_file);
    fclose(rom_file);
    if (bytes_read != c.rom_size) {
        LOG_ERROR(LOG_MAIN, "FAILED TO READ ENTIRE ROM");
        free(c.rom_data);
        return false;
    }

    c.header = (struct rom_header*)(c.rom_data + 0x100);
    if (!validate_header(c.header)) {
        LOG_ERROR(LOG_MAIN, "INVALID ROM HEADER");
        free(c.rom_data);
        return false;
    }

    c.header->title[15] = '\0';
    describe_cartridge(&c);

    if (!validate_checksum(c.rom_data, c.header->checksum)) {
        LOG_WARN(LOG_MAIN, "HEADER CHECKSUM VALIDATION FAILED");
    }

    if (!validate_global_checksum(c.rom_data, c.rom_size)) {
        LOG_WARN(LOG_MAIN, "GLOBAL CHECKSUM VALIDATION FAILED");
    }

    // DETERMINE ROM SIZE
    u32 rom_banks;
    switch(c.header->size_rom) {
        case 0x00: rom_banks = 2; break;    // 32KB
        case 0x01: rom_banks = 4; break;    // 64KB
        case 0x02: rom_banks = 8; break;    // 128KB
        case 0x03: rom_banks = 16; break;   // 256KB
        case 0x04: rom_banks = 32; break;   // 512KB
        case 0x05: rom_banks = 64; break;   // 1MB
        case 0x06: rom_banks = 128; break;  // 2MB
        case 0x07: rom_banks = 256; break;  // 4MB
        case 0x08: rom_banks = 512; break;  // 8MB
        default:
            LOG_ERROR(LOG_MAIN, "INVALID ROM SIZE CODE: 0x%02X", c.header->size_rom);
            return false;
    }

    // RAM SIZE AND BATTERY
    c.has_battery = false;
    c.has_rtc = false;
    
    switch(c.header->type) {
        case NO_MBC:
            c.mbc_type = NO_MBC;
            break;
            
        case MBC1:
        case MBC1_RAM:
        case MBC1_RAM_BATTERY:
            c.mbc_type = c.header->type;
            c.has_battery = (c.header->type == MBC1_RAM_BATTERY);
            break;
            
        case MBC2:
        case MBC2_BATTERY:
            c.mbc_type = c.header->type;
            c.has_battery = (c.header->type == MBC2_BATTERY);
            c.ram_size = 512;  // FIXED 512x4 BIT RAM
            break;
            
        case MBC3_TIMER_BATTERY:
        case MBC3_TIMER_RAM_BATTERY:
        case MBC3:
        case MBC3_RAM:
        case MBC3_RAM_BATTERY:
            c.mbc_type = c.header->type;
            c.has_battery = (c.header->type == MBC3_RAM_BATTERY ||
                           c.header->type == MBC3_TIMER_BATTERY ||
                           c.header->type == MBC3_TIMER_RAM_BATTERY);
            c.has_rtc = (c.header->type == MBC3_TIMER_BATTERY ||
                        c.header->type == MBC3_TIMER_RAM_BATTERY);
            break;
            
        case MBC5:
        case MBC5_RAM:
        case MBC5_RAM_BATTERY:
            c.mbc_type = c.header->type;
            c.has_battery = (c.header->type == MBC5_RAM_BATTERY);
            break;
            
        default:
            LOG_ERROR(LOG_MAIN, "UNSUPPORTED CART TYPE: 0x%02X", c.header->type);
            return false;
    }

    // ALLOCATE RAM IF NEEDED (EXCEPT MBC2 WHICH WAS HANDLED ABOVE)
    if (c.mbc_type != MBC2 && c.mbc_type != MBC2_BATTERY) {
        switch(c.header->size_ram) {
            case 0x00: c.ram_size = 0; break;
            case 0x01: c.ram_size = 2048; break;     // 2KB
            case 0x02: c.ram_size = 8192; break;     // 8KB
            case 0x03: c.ram_size = 32768; break;    // 32KB
            case 0x04: c.ram_size = 131072; break;   // 128KB
            case 0x05: c.ram_size = 65536; break;    // 64KB
            default:
                LOG_WARN(LOG_MAIN, "UNKNOWN RAM SIZE CODE: 0x%02X", c.header->size_ram);
                c.ram_size = 0;
        }
    }

    // ALLOCATE RAM IF NEEDED
    if (c.ram_size > 0) {
        c.ram_data = calloc(1, c.ram_size);
        if (!c.ram_data) {
            LOG_ERROR(LOG_MAIN, "FAILED TO ALLOCATE RAM: %u BYTES", c.ram_size);
            free(c.rom_data);
            return false;
        }
        
        // LOAD BATTERY SAVE IF PRESENT
        if (c.has_battery) {
            char save_file[1024];
            snprintf(save_file, sizeof(save_file), "%s.sav", cart);
            FILE* f = fopen(save_file, "rb");
            if (f) {
                fread(c.ram_data, 1, c.ram_size, f);
                fclose(f);
                LOG_INFO(LOG_MAIN, "LOADED SAVE FILE: %s", save_file);
            }
        }
    }

    // INIT BANKING
    c.current_rom_bank = 1;
    c.current_ram_bank = 0;
    c.banking_mode = 0;
    c.ram_enabled = false;

    // LOADING IF POSSIBLE
    load_battery();

    LOG_INFO(LOG_MAIN, "CARTRIDGE LOADED: TYPE=%02X RAM=%uKB BATTERY=%d RTC=%d",
            c.mbc_type, c.ram_size/1024, c.has_battery, c.has_rtc);
    return true;
}

u8 read_cart(u16 addr) {
    // GET CORRECT ROM OFFSET BASED ON MBC TYPE
    u32 offset = get_rom_offset(addr);
    if (offset >= c.rom_size) {
        LOG_ERROR(LOG_BUS, "ROM READ OUT OF BOUNDS: ADDR=0x%04X BANK=%02X OFFSET=0x%08X", 
                addr, c.current_rom_bank, offset);
        return 0xFF;
    }
    return c.rom_data[offset];
}

u8 read_cart_ram(u16 addr) {
    // CHECK RAM ACCESS
    if (!c.ram_enabled || !c.ram_data) {
        return 0xFF;
    }

    // GET CORRECT RAM OFFSET BASED ON MBC TYPE
    u32 offset = get_ram_offset(addr);
    if (offset >= c.ram_size) {
        LOG_ERROR(LOG_BUS, "RAM READ OUT OF BOUNDS: ADDR=0x%04X BANK=%02X OFFSET=0x%08X",
                addr, c.current_ram_bank, offset);
        return 0xFF; 
    }

    return c.ram_data[offset];
}

void write_to_cart(u16 addr, u8 val) {
    // RAM ENABLE (ALL MBCS)
    if (addr < 0x2000) {
        c.ram_enabled = ((val & 0x0F) == 0x0A);
        return;
    }

    // ROM BANK SELECT
    if (addr < 0x4000) {
        if (is_mbc1()) {
            val &= 0x1F;
            if (val == 0) val = 1;
            c.current_rom_bank = (c.current_rom_bank & 0xE0) | val;
        }
        else if (is_mbc3()) {
            val &= 0x7F;
            if (val == 0) val = 1;
            c.current_rom_bank = val;
        }
        return;
    }

    // RAM BANK SELECT
    if (addr < 0x6000) {
        if (is_mbc1()) {
            if (c.banking_mode == 0) {
                c.current_rom_bank = (c.current_rom_bank & 0x1F) | ((val & 0x03) << 5);
            } else {
                c.current_ram_bank = val & 0x03;
            }
        }
        else if (is_mbc3()) {
            if (val <= 0x03) {
                c.current_ram_bank = val;
            }
            // RTC REGISTERS 08-0C HANDLED HERE IF NEEDED
        }
        return;
    }

    // BANKING MODE SELECT (MBC1 ONLY)
    if (addr < 0x8000 && is_mbc1()) {
        c.banking_mode = val & 0x01;
    }
}

void write_cart_ram(u16 addr, u8 val) {
    // CHECK RAM ACCESS PERMISSION
    if (!c.ram_enabled || !c.ram_data) {
        return;
    }

    // GET CORRECT RAM OFFSET BASED ON MBC TYPE 
    u32 offset = get_ram_offset(addr);
    if (offset >= c.ram_size) {
        LOG_ERROR(LOG_BUS, "RAM WRITE OUT OF BOUNDS: ADDR=0x%04X BANK=%02X OFFSET=0x%08X",
                addr, c.current_ram_bank, offset);
        return;
    }

    LOG_TRACE(LOG_BUS, "RAM WRITE: [0x%04X] <- 0x%02X (BANK=%02X)", 
            addr, val, c.current_ram_bank);
    c.ram_data[offset] = val;
}

void describe_cartridge(const struct cartridge* cart) {
    if (!cart || !cart->rom_data || !cart->header) {
        LOG_ERROR(LOG_MAIN, "INVALID CARTRIDGE DATA");
        return;
    }

    LOG_INFO(LOG_MAIN, "CARTRIDGE INFO:");
    LOG_INFO(LOG_MAIN, "  FILENAME: %s", cart->filename);
    LOG_INFO(LOG_MAIN, "  ROM SIZE: %u BYTES", cart->rom_size);
    LOG_INFO(LOG_MAIN, "  TITLE: %s", cart->header->title);
    LOG_INFO(LOG_MAIN, "  TYPE: 0x%02X", cart->header->type);
    LOG_INFO(LOG_MAIN, "  ROM SIZE CODE: 0x%02X", cart->header->size_rom);
    LOG_INFO(LOG_MAIN, "  RAM SIZE CODE: 0x%02X", cart->header->size_ram);
    LOG_INFO(LOG_MAIN, "  LICENSE: 0x%02X", cart->header->license);
    LOG_INFO(LOG_MAIN, "  VERSION: 0x%02X", cart->header->version);

    LOG_DEBUG(LOG_MAIN, "ENTRY POINT: %02X %02X %02X %02X",
        cart->header->entry[0], cart->header->entry[1],
        cart->header->entry[2], cart->header->entry[3]);
}

bool validate_header(const struct rom_header* header) {
    if (!header) {
        return false;
    }

    const size_t MIN_ROM_SIZE = 0x0150;
    if (c.rom_size < MIN_ROM_SIZE) {
        LOG_ERROR(LOG_MAIN, "ROM TOO SMALL: %u BYTES", c.rom_size);
        return false;
    }

    if (header->size_rom > 0x08) {
        LOG_WARN(LOG_MAIN, "INVALID ROM SIZE CODE: 0x%02X", header->size_rom);
    }

    if (header->size_ram > 0x05) {
        LOG_WARN(LOG_MAIN, "INVALID RAM SIZE CODE: 0x%02X", header->size_ram);
    }

    if (header->dest != 0x00 && header->dest != 0x01) {
        LOG_WARN(LOG_MAIN, "INVALID DESTINATION CODE: 0x%02X", header->dest);
    }

    return true;
}

bool validate_checksum(const u8* rom_data, u8 expected_checksum) {
    if (!rom_data) {
        LOG_ERROR(LOG_MAIN, "INVALID ROM DATA FOR CHECKSUM");
        return false;
    }

    u16 check = 0;
    for (u16 i = 0x0134; i <= 0x014C; i++) {
        check = check - rom_data[i] - 1;
    }

    if ((check & 0xFF) != expected_checksum) {
        LOG_ERROR(LOG_MAIN, "CHECKSUM MISMATCH: GOT=0x%02X EXPECTED=0x%02X",
                (check & 0xFF), expected_checksum);
        return false;
    }

    LOG_INFO(LOG_MAIN, "CHECKSUM VALIDATED SUCCESSFULLY");
    return true;
}

bool validate_global_checksum(const u8* rom_data, u32 rom_size) {
    if (!rom_data || rom_size < 0x150) {
        LOG_ERROR(LOG_MAIN, "INVALID ROM DATA OR SIZE FOR GLOBAL CHECKSUM");
        return false;
    }

    u16 g_checksum = (c.rom_data[0x014E] << 8) | c.rom_data[0x014F];
    u32 check = 0;

    for (u32 i = 0; i < rom_size; i++) {
        if (i != 0x014E && i != 0x014F) {
            check += rom_data[i];
        }
    }

    check &= 0xFFFF;

    if (check != g_checksum) {
        LOG_ERROR(LOG_MAIN, "GLOBAL CHECKSUM MISMATCH: GOT=0x%04X EXPECTED=0x%04X",
                check, g_checksum);
        return false;
    }

    LOG_INFO(LOG_MAIN, "GLOBAL CHECKSUM VALIDATED SUCCESSFULLY");
    return true;
}

bool save_battery(void) {
    // NO BATTERY = NO SAVE
    if (!c.has_battery || !c.ram_data || c.ram_size == 0) {
        return false;
    }

    // CREATE SAVE FILENAME
    char save_file[1024];
    snprintf(save_file, sizeof(save_file), "%s.sav", c.filename);

    // OPEN SAVE FILE
    FILE* f = fopen(save_file, "wb");
    if (!f) {
        LOG_ERROR(LOG_CART, "FAILED TO CREATE SAVE FILE: %s", save_file);
        return false;
    }

    // WRITE RAM
    size_t written = fwrite(c.ram_data, 1, c.ram_size, f);
    if (written != c.ram_size) {
        LOG_ERROR(LOG_CART, "FAILED TO WRITE SAVE DATA");
        fclose(f);
        return false;
    }

    // WRITE RTC IF PRESENT
    if (c.has_rtc) {
        fwrite(c.rtc_reg, 1, sizeof(c.rtc_reg), f);
        fwrite(&c.rtc_last, 1, sizeof(c.rtc_last), f);
    }

    fclose(f);
    LOG_INFO(LOG_CART, "SAVED BATTERY TO: %s", save_file);
    return true;
}

bool load_battery(void) {
    // NO BATTERY = NO LOAD
    if (!c.has_battery || !c.ram_data || c.ram_size == 0) {
        return false;
    }

    // GET SAVE FILENAME
    char save_file[1024];
    snprintf(save_file, sizeof(save_file), "%s.sav", c.filename);

    // TRY OPEN SAVE
    FILE* f = fopen(save_file, "rb");
    if (!f) {
        LOG_INFO(LOG_CART, "NO SAVE FILE FOUND: %s", save_file);
        return false;
    }

    // READ RAM
    size_t read = fread(c.ram_data, 1, c.ram_size, f);
    if (read != c.ram_size) {
        LOG_ERROR(LOG_CART, "FAILED TO READ SAVE DATA");
        fclose(f);
        return false;
    }

    // READ RTC IF PRESENT
    if (c.has_rtc) {
        fread(c.rtc_reg, 1, sizeof(c.rtc_reg), f);
        fread(&c.rtc_last, 1, sizeof(c.rtc_last), f);
    }

    fclose(f);
    LOG_INFO(LOG_CART, "LOADED BATTERY FROM: %s", save_file);
    return true;
}
