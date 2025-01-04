#include "cart.h"

// ZERO FOR SAFETY
static struct cartridge c = {0};

// CART TYPES, NOT EXHAUSTIVE
static const char* ROMS[] = {
    "ROM ONLY",
    "MBC1",
    "MBC1+RAM",
    "MBC1+RAM+BATTERY",
    "0x04 ???",
    "MBC2",
    "MBC2+BATTERY",
    "0x07 ???",
    "ROM+RAM 1",
    "ROM+RAM+BATTERY 1",
    "0x0A ???",
    "MMM01",
    "MMM01+RAM",
    "MMM01+RAM+BATTERY",
    "0x0E ???",
    "MBC3+TIMER+BATTERY",
    "MBC3+TIMER+RAM+BATTERY 2",
    "MBC3",
    "MBC3+RAM 2",
    "MBC3+RAM+BATTERY 2",
    "0x14 ???",
    "0x15 ???",
    "0x16 ???",
    "0x17 ???",
    "0x18 ???",
    "MBC5",
    "MBC5+RAM",
    "MBC5+RAM+BATTERY",
    "MBC5+RUMBLE",
    "MBC5+RUMBLE+RAM",
    "MBC5+RUMBLE+RAM+BATTERY",
    "0x1F ???",
    "MBC6",
    "0x21 ???",
    "MBC7+SENSOR+RUMBLE+RAM+BATTERY",
};

// LICENSE BIT VALIDATION
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


bool load_cartridge(const char* cart){
    printf("Loading Cartridge...\n");

    // OPEN ROM FILE
    snprintf(c.filename, sizeof(c.filename), "%s", cart);
    FILE* rom_file = fopen(cart, "rb");
    if (!rom_file){
        fprintf(stderr, "Error: Failed to open file '%s'\n", cart);
        return false;
    }

    // GET ROM SIZE - REWIND SEEK TO START
    fseek(rom_file, 0, SEEK_END);
    long file_size = ftell(rom_file);
    if (file_size <= 0) {
        fprintf(stderr, "Error: Invalid ROM file size\n");
        fclose(rom_file);
        return false;
    }
    rewind(rom_file);
    c.rom_size = (u32)file_size;

    // ALLOCATE ROM MEMORY
    c.rom_data = malloc(c.rom_size);
    if (!c.rom_data) {
        fprintf(stderr, "Error: Failed to allocate memory for ROM data\n");
        fclose(rom_file);
        return false;
    }

    // READ ROM DATE INTO MEMORY - CLOSE FILE
    size_t bytes_read = fread(c.rom_data, 1, c.rom_size, rom_file);
    fclose(rom_file);
    if (bytes_read != c.rom_size) {
        fprintf(stderr, "Error: Failed to read entire ROM file\n");
        free(c.rom_data);
        return false;
    }

    // MAP ROM HEADER STARTING AT OFFSET - VALIDATE HEADER DATA
    c.header = (struct rom_header*)(c.rom_data + 0x100);
    if (!validate_header(c.header)) {
        fprintf(stderr, "Error: Invalid ROM header\n");
        free(c.rom_data);
        return false;
    }

    // NULL TERMINATE TITLE
    c.header->title[15] = '\0';

    /* UNCOMMENT FOR NEWER TARGET
    c.header->man_code[3] = '\0'; */

    // LOG CONTENTS OF LOADED CARTRIDGE
    describe_cartridge(&c);

    // LOOP THROUGH CHECKSUM RANGE IN ROM DATA, NEGATIVE SUM - 1, MASK AND VALIDATE REMAINING BYTE
    if (!validate_checksum(c.rom_data, c.header->checksum)) {
        fprintf(stderr, "Warning: Header checksum validation FAILED\n");
    }

    // LOOP THROUGH ALL BYTES EXCEPT GLOBAL CS BYTE LOCATION, MASK AND VALIDATE REMAINING 16 BITS
    if (!validate_global_checksum(c.rom_data, c.rom_size)) {
        fprintf(stderr, "Warning: Global checksum validation FAILED\n");
    }

    printf("Cartridge loaded successfully.\n");
    return true;
}

void describe_cartridge(const struct cartridge* cart) {
    if (!cart || !cart->rom_data || !cart->header) {
        printf("Error: Invalid cartridge data.\n");
        return;
    }

    // GENERAL
    printf("\n--- GENERAL ---\n");
    printf("FILENAME: %s\n", cart->filename);
    printf("ROM SIZE: %u BYTES\n", cart->rom_size);

    // HEADER
    printf("\n--- HEADER ---\n");
    printf("ENTRY POINT:\n");
    for (int i = 0; i < 4; i++) {
        printf("%02X ", cart->header->entry[i]);
    }
    printf("\nLOGO (FIRST 16 BYTES):\n");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", cart->header->logo[i]);
    }
    printf("...\n");
    printf("TITLE: %s\n", cart->header->title);
    /* UNCOMMENT FOR NEWER TARGET SYSTEM, SHOULD PROBABLY DO THIS PROGRAMATICALLY
    printf("MANUFACTURER CODE: %s\n", cart->header->man_code);
    printf("COLOR GAMEBOY FLAG: 0x%02X\n", cart->header->cgb_flag); */
    printf("NEW LICENSE: 0x%04X\n", cart->header->license_new);
    printf("SUPER GAMEBOY: 0x%02X\n", cart->header->sgb);
    printf("CARTRIDGE TYPE: 0x%02X\n", cart->header->type);
    printf("ROM SIZE CODE: 0x%02X\n", cart->header->size_rom);
    printf("RAM SIZE CODE: 0x%02X\n", cart->header->size_ram);
    printf("DESTINATION CODE: 0x%02X\n", cart->header->dest);
    printf("OLD LICENSE: 0x%02X\n", cart->header->license);
    printf("VERSION: 0x%02X\n", cart->header->version);
    printf("HEADER CHECKSUM: 0x%02X\n", cart->header->checksum);
    printf("\n\n--- END ---\n\n");
}

bool validate_header(const struct rom_header* header) {
    if (!header) {
        return false;
    }

    // MINIMUM ROM FILE SIZE
    const size_t MIN_ROM_SIZE = 0x0150;
    if (c.rom_size < MIN_ROM_SIZE) {
        fprintf(stderr, "Error: ROM file is too small (size: %u bytes)\n", c.rom_size);
        return false;
    }

    /* UNCOMMENT FOR NEWER TARGET SYSTEM, SHOULD PROBABLY DO THIS PROGRAMATICALLY
    // CBG FLAG (0x00, 0x80, OR 0xC0)
    if (header->cgb_flag != 0x00 && header->cgb_flag != 0x80 && header->cgb_flag != 0xC0) {
        fprintf(stderr, "Warning: Invalid CGB flag: 0x%02X\n", header->cgb_flag);
    } */

    // ROM SIZE CODE (0x00 - 0x08)
    if (header->size_rom > 0x08) {
        fprintf(stderr, "Warning: Invalid ROM size code: 0x%02X\n", header->size_rom);
    }

    // RAM SIZE CODE (0x00 - 0x05)
    if (header->size_ram > 0x05) {
        fprintf(stderr, "Warning: Invalid RAM size code: 0x%02X\n", header->size_ram);
    }

    // DESTINATION CODE (0x00 OR 0x01)
    if (header->dest != 0x00 && header->dest != 0x01) {
        fprintf(stderr, "Warning: Invalid destination code: 0x%02X\n", header->dest);
    }

    return true;
}

bool validate_checksum(const u8* rom_data, u8 expected_checksum) {
    if (!rom_data) {
        fprintf(stderr, "Error: Invalid ROM data\n");
        return false;
    }

    u16 check = 0;
    for (u16 i = 0x0134; i <= 0x014C; i++) {
        check = check - rom_data[i] - 1;
    }

    if ((check & 0xFF) != expected_checksum) {
        fprintf(stderr, "Error: Checksum isn't valid (Returned: 0x%02X, Expected: 0x%02X)\n",
                (check & 0xFF), expected_checksum);
        return false;
    }

    printf("Checksum successfully validated\n");
    return true;
}

bool validate_global_checksum(const u8* rom_data, u32 rom_size) {
    if (!rom_data || rom_size < 0x150) {
        fprintf(stderr, "Error: Invalid ROM data or size\n");
        return false;
    }

    // GET CHECKSUM BYTES DIRECTLY FROM ROM DATA
    u16 g_checksum = (c.rom_data[0x014E] << 8) | c.rom_data[0x014F];
    u32 check = 0;

    // SUM OF ALL BYTES, EXCLUDING 0x014E AND 0x014F
    for (u32 i = 0; i < rom_size; i++) {
        if (i != 0x014E && i != 0x014F) {
            check += rom_data[i];
        }
    }

    // MASK TO 16 BIT BE
    check &= 0xFFFF;

    if (check != g_checksum) {
        fprintf(stderr, "Error: Global checksum isn't valid (Returned: 0x%04X, Expected: 0x%04X)\n",
                check, g_checksum);
        return false;
    }

    printf("Global checksum successfully validated\n");
    return true;
}

u8 read_cart(u16 addr) {
    if (addr < ROM_BANK) {
        return c.rom_data[addr];  // FIXED BANK
    } else {
        // TODO: HANDLE BANK SWITCHING WHEN IMPLEMENTED
        return c.rom_data[addr];  // RN ONLY RETURNS FROM BANK 1
    }
}

void write_to_cart(u16 addr, u8 val) {
    // TODO: HANDLE MBC (MEMORY BANK CONTROLLER) OPERATIONS (USED FOR BANK SWITCHING)
}

u8 read_cart_ram(u16 addr) {
    if (!c.ram_enabled) {
        return 0xFF;    // IS DISABLED
    }
    
    if (!c.ram_data) {
        return 0xFF;    // NONE PRESENT
    }

    // TODO: HANDLE RAM BANKING WHEN IMPLEMENTED
    return c.ram_data[addr];
}

void write_cart_ram(u16 addr, u8 val) {
    if (!c.ram_enabled) {
        return;         // IS DISABLED
    }
    
    if (!c.ram_data) {
        return;         // NONE PRESENT
    }

    // TODO: HANDLE RAM BANKING WHEN IMPLEMENTED
    c.ram_data[addr] = val;
}
