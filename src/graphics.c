#include "logger.h"
#include "graphics.h"
#include "interrupt.h"
#include "bus.h"
#include "joypad.h"
#include "cart.h"
#include "ui.h"
#include <string.h>

static graphics_system graphics;

static SDL_HitTestResult SDLCALL sdl_hit_test_cb(SDL_Window* w,
                                                 const SDL_Point* p,
                                                 void* data) {
    (void)data;
    int win_w, win_h;
    SDL_GetWindowSize(w, &win_w, &win_h);
    return ui_hit_test(p->x, p->y, win_w, win_h);
}

// INITIAL PALETTE
static const u32 DEFAULT_COLORS[4] = {
    0xFFFFFFFF,  // WHITE (00)
    0xFFAAAAAA,  // LIGHT GRAY (01) 
    0xFF555555,  // DARK GRAY (10)
    0xFF000000   // BLACK (11)
};

static void update_palette(u8* palette_reg, u32* colors, u8 val) {
    *palette_reg = val;
    // GET 2 BIT COLOR VAL AND MAP TO CORRECT SHADE    BITS
    colors[0] = DEFAULT_COLORS[(val & 0x03)];         // 1-0
    colors[1] = DEFAULT_COLORS[(val >> 2) & 0x03];    // 3-2  
    colors[2] = DEFAULT_COLORS[(val >> 4) & 0x03];    // 5-4
    colors[3] = DEFAULT_COLORS[(val >> 6) & 0x03];    // 7-6
}

void graphics_cleanup() {
    ui_shutdown();
    SDL_DestroyTexture(graphics.texture);
    SDL_DestroyRenderer(graphics.renderer);
    SDL_DestroyWindow(graphics.window);

    if (graphics.debug_texture) {
        SDL_DestroyTexture(graphics.debug_texture);
    }
    if (graphics.debug_renderer) {
        SDL_DestroyRenderer(graphics.debug_renderer);
    }
    if (graphics.debug_window) {
        SDL_DestroyWindow(graphics.debug_window);
    }

    SDL_Quit();
}

#if DEBUG_WINDOW
static u8 get_tile_pixel(u8 tile_idx, u8 x, u8 y) {
    u16 tile_addr = (tile_idx * 16) + (y * 2);

    // READ TILE DATA BYTES
    u8 byte1 = read_from_bus(VRAM_START + tile_addr);
    u8 byte2 = read_from_bus(VRAM_START + tile_addr + 1);

    // GET BIT POSITION FROM X
    u8 bit_pos = 7 - x;

    // COMBINE BITS TO GET COLOR INDEX
    return ((byte2 >> bit_pos) & 1) << 1 | ((byte1 >> bit_pos) & 1);
}
#endif

static void render_background_line() {
    // PREP Y-COORDINATE CALCULATIONS
    const u8 bg_y = (graphics.line + graphics.scy) & 0xFF; // CURRENT LINE IN BG MAP
    const u8 tile_row = bg_y / 8;       // CURRENT ROW IN TILE MAP
    const u8 tile_y_offset = bg_y % 8; // CURRENT Y OFFSET IN TILE

    // PREP MAP ADDRESSES
    const u16 BG_MAP_ADDRESSES[] = {0x9800, 0x9C00};
    const u16 bg_map_base = BG_MAP_ADDRESSES[(graphics.lcdc & LCDC_BG_MAP) >> 3];

    const u16 TILE_DATA_ADDRESSES[] = {0x8800, 0x8000};
    const u16 tile_data_base = TILE_DATA_ADDRESSES[(graphics.lcdc & LCDC_TILE_SELECT) >> 4];
    const bool is_signed_addressing = (tile_data_base == 0x8800);

    // RENDER BG
    
    if (graphics.lcdc & LCDC_BG_ENABLE) {
        for (int screen_x = 0; screen_x < SCREEN_WIDTH; screen_x++) {
            const u8 bg_x = (screen_x + graphics.scx) & 0xFF;
            const u8 tile_col = bg_x / 8;
            const u8 tile_x_offset = bg_x % 8;

            const u16 map_addr = bg_map_base + (tile_row * 32) + tile_col;
            u8 tile_idx = read_from_bus(map_addr);

            if (is_signed_addressing) {
                tile_idx = ((i8)tile_idx + 128) & 0xFF;
            }

            const u16 pixel_addr = tile_data_base + (tile_idx * 16) + (tile_y_offset * 2);
            const u8 low_byte = read_from_bus(pixel_addr);
            const u8 high_byte = read_from_bus(pixel_addr + 1);

            const u8 bit_pos = 7 - tile_x_offset;
            const u8 color_idx = ((high_byte >> bit_pos) & 1) << 1 | ((low_byte >> bit_pos) & 1);

            graphics.frame_buffer[(graphics.line * SCREEN_WIDTH) + screen_x] = graphics.bg_colors[color_idx];
            graphics.bg_index_line[screen_x] = color_idx;
        }
    } else {
        for (int x = 0; x < SCREEN_WIDTH; x++) graphics.bg_index_line[x] = 0;
    }

    if ((graphics.lcdc & LCDC_WINDOW_ENABLE) && graphics.wy <= graphics.line && graphics.wx < 167) {
        const u8 window_y = graphics.window_line;
        const u8 tile_row = window_y / 8;
        const u8 tile_y_offset = window_y % 8;
        graphics.window_line++;

        const u16 window_map_base = BG_MAP_ADDRESSES[(graphics.lcdc & LCDC_WINDOW_MAP) >> 6];

        for (int screen_x = 0; screen_x < SCREEN_WIDTH; screen_x++) {
            if (screen_x + 7 < graphics.wx) continue;

            const u8 window_x = screen_x - (graphics.wx - 7);
            const u8 tile_col = window_x / 8;
            const u8 tile_x_offset = window_x % 8;

            const u16 map_addr = window_map_base + (tile_row * 32) + tile_col;
            u8 tile_idx = read_from_bus(map_addr);

            if (is_signed_addressing) {
                tile_idx = ((i8)tile_idx + 128);
            }

            const u16 pixel_addr = tile_data_base + (tile_idx * 16) + (tile_y_offset * 2);
            const u8 low_byte = read_from_bus(pixel_addr);
            const u8 high_byte = read_from_bus(pixel_addr + 1);

            const u8 bit_pos = 7 - tile_x_offset;
            const u8 color_idx = ((high_byte >> bit_pos) & 1) << 1 | ((low_byte >> bit_pos) & 1);

            graphics.frame_buffer[(graphics.line * SCREEN_WIDTH) + screen_x] = graphics.bg_colors[color_idx];
            graphics.bg_index_line[screen_x] = color_idx;
        }
    }
}


// RENDER SPRITES FOR CURRENT SCAN LINE
static void render_sprites_line() {
    if (!(graphics.lcdc & LCDC_OBJ_ENABLE)) return;

    const u8 sprite_height = (graphics.lcdc & LCDC_OBJ_SIZE) ? 16 : 8;

    sprite line_sprites[SPRITES_PER_LINE];
    int sprite_count = 0;

    for (int i = 0; i < MAX_SPRITES && sprite_count < SPRITES_PER_LINE; i++) {
        const int sprite_y = graphics.oam[i].y - 16;
        if (graphics.line >= sprite_y && graphics.line < sprite_y + sprite_height) {
            line_sprites[sprite_count++] = graphics.oam[i];
        }
    }

    for (int i = 1; i < sprite_count; i++) {
        sprite key = line_sprites[i];
        int j = i - 1;
        while (j >= 0 && line_sprites[j].x > key.x) {
            line_sprites[j + 1] = line_sprites[j];
            j--;
        }
        line_sprites[j + 1] = key;
    }

    for (int i = sprite_count - 1; i >= 0; i--) {
        const sprite* s = &line_sprites[i];
        const int sprite_x = (int)s->x - 8;

        if (s->x == 0 || sprite_x >= SCREEN_WIDTH) continue;

        const bool y_flip = (s->flags & 0x40) != 0;
        const u8 palette = (s->flags & 0x10) ? 1 : 0;
        const bool bg_priority = (s->flags & 0x80) != 0;

        u8 sprite_line = (u8)((int)graphics.line - ((int)s->y - 16));
        if (y_flip) sprite_line = (sprite_height - 1) - sprite_line;

        u8 tile_index = s->tile;
        if (sprite_height == 16) {
            tile_index &= 0xFE;
            if (sprite_line >= 8) {
                tile_index++;
                sprite_line -= 8;
            }
        }

        const u16 tile_data_address = VRAM_START + (tile_index * 16) + (sprite_line * 2);
        const u8 low_byte  = read_from_bus(tile_data_address);
        const u8 high_byte = read_from_bus(tile_data_address + 1);

        for (int px = 0; px < 8; px++) {
            const int screen_x = sprite_x + px;
            if (screen_x < 0 || screen_x >= SCREEN_WIDTH) continue;

            const u8 bit_pos = (s->flags & 0x20) ? px : (7 - px);
            const u8 color_idx = (((high_byte >> bit_pos) & 1) << 1) | ((low_byte >> bit_pos) & 1);
            if (color_idx == 0) continue;

            if (bg_priority && graphics.bg_index_line[screen_x] != 0) continue;

            const int fb_index = (graphics.line * SCREEN_WIDTH) + screen_x;
            graphics.frame_buffer[fb_index] = graphics.sprite_colors[palette][color_idx];
        }
    }
}

void render_line() {
    if (!(graphics.lcdc & LCDC_ENABLE)) {
        const int fb_start = graphics.line * SCREEN_WIDTH;
        const u32 white = graphics.bg_colors[0];
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            graphics.frame_buffer[fb_start + x] = white;
        }
        return;
    }

    if (graphics.line >= SCREEN_HEIGHT) return;

    const u32 bg_color0 = graphics.bg_colors[0];
    const int fb_start = graphics.line * SCREEN_WIDTH;
    memset(&graphics.frame_buffer[fb_start], bg_color0, SCREEN_WIDTH * sizeof(u32));

    render_background_line();
    render_sprites_line();
}

static SDL_Rect compute_gb_dst_rect(void) {
    int win_w, win_h;
    SDL_GetWindowSize(graphics.window, &win_w, &win_h);
    return ui_screen_rect(win_w, win_h);
}

void draw_frame() {
#if DEBUG_WINDOW
    static int frame_count = 0;
    frame_count++;
#endif

    ui_new_frame();

    int win_w, win_h;
    SDL_GetWindowSize(graphics.window, &win_w, &win_h);

    SDL_SetRenderDrawColor(graphics.renderer, 0, 0, 0, 255);
    SDL_RenderClear(graphics.renderer);

    ui_draw_chrome_under(graphics.renderer, win_w, win_h);

    SDL_Rect dst = compute_gb_dst_rect();

    if (c.rom_data) {
        void* pixels;
        int pitch;
        if (SDL_LockTexture(graphics.texture, NULL, &pixels, &pitch) < 0) {
            LOG_ERROR(LOG_GRAPHICS, "Failed to lock texture: %s\n", SDL_GetError());
            return;
        }
        memcpy(pixels, graphics.frame_buffer, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u32));
        SDL_UnlockTexture(graphics.texture);
        SDL_RenderCopy(graphics.renderer, graphics.texture, NULL, &dst);
    } else {
        ui_draw_splash(graphics.renderer, dst);
    }

    ui_render();

    ui_draw_chrome_over(graphics.renderer, win_w, win_h);

    SDL_RenderPresent(graphics.renderer);

#if DEBUG_WINDOW
    if (frame_count % 30 == 0) {
        update_debug_window();
    }
#endif
}

static SDL_Surface* make_app_icon(void) {
    const int W = 64, H = 64;
    static u32 px[64 * 64];

    const u32 BG     = 0xFF1F1F2E;
    const u32 SHELL  = 0xFFCCCCD0;
    const u32 SCREEN = 0xFF9BBC0F;
    const u32 INK    = 0xFF0F380F;

    for (int i = 0; i < W * H; i++) px[i] = BG;

    for (int y = 3; y < 61; y++) {
        for (int x = 3; x < 61; x++) {
            int dx = (x < 11) ? (11 - x) : (x > 52 ? x - 52 : 0);
            int dy = (y < 11) ? (11 - y) : (y > 52 ? y - 52 : 0);
            if (dx * dx + dy * dy <= 8 * 8) px[y * W + x] = SHELL;
        }
    }

    for (int y = 10; y < 38; y++)
        for (int x = 12; x < 52; x++)
            px[y * W + x] = SCREEN;

    static const u8 G[7] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E};
    static const u8 B[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
    int gx = 18, gy = 16, bx = 30, by = 16;
    for (int r = 0; r < 7; r++) {
        for (int c = 0; c < 5; c++) {
            if (G[r] & (1 << (4 - c))) {
                px[(gy + r) * W + (gx + c)] = INK;
                px[(gy + r) * W + (gx + c) + 1] = INK;
            }
            if (B[r] & (1 << (4 - c))) {
                px[(by + r) * W + (bx + c)] = INK;
                px[(by + r) * W + (bx + c) + 1] = INK;
            }
        }
    }

    for (int y = 43; y < 53; y++) {
        for (int x = 35; x < 55; x++) {
            int dxA = x - 41, dyA = y - 48;
            int dxB = x - 51, dyB = y - 46;
            if (dxA * dxA + dyA * dyA <= 9) px[y * W + x] = 0xFFB04060;
            if (dxB * dxB + dyB * dyB <= 9) px[y * W + x] = 0xFFB04060;
        }
    }
    for (int y = 44; y < 53; y++) px[y * W + 14] = 0xFF222226;
    for (int y = 44; y < 53; y++) px[y * W + 15] = 0xFF222226;
    for (int y = 44; y < 53; y++) px[y * W + 16] = 0xFF222226;
    for (int x = 11; x < 20; x++) px[48 * W + x] = 0xFF222226;
    for (int x = 11; x < 20; x++) px[49 * W + x] = 0xFF222226;

    return SDL_CreateRGBSurfaceFrom(
        px, W, H, 32, W * sizeof(u32),
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
}

void graphics_update_title_for_cart(void) {
    if (!graphics.window || !c.filename[0]) return;
    const char* base = strrchr(c.filename, '/');
    base = base ? base + 1 : c.filename;
    char title[1100];
    snprintf(title, sizeof(title), "%s - %s", EMU_TITLE, base);
    char* dot = strrchr(title, '.');
    if (dot && dot > strrchr(title, '/')) *dot = '\0';
    SDL_SetWindowTitle(graphics.window, title);
}

void graphics_init() {
    SDL_SetHint(SDL_HINT_APP_NAME, EMU_TITLE);
#ifdef SDL_HINT_VIDEO_WAYLAND_WMCLASS
    SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_WMCLASS, EMU_APP_ID);
#endif
#ifdef SDL_HINT_VIDEO_X11_WMCLASS
    SDL_SetHint(SDL_HINT_VIDEO_X11_WMCLASS, EMU_APP_ID);
#endif

    // INIT SDL WITH ERROR CHECKING
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        LOG_ERROR(LOG_GRAPHICS, "SDL INIT FAILED: %s\n", SDL_GetError());
        exit(1);
    }

    // PPU STATE INIT - SHARED WITH RESET
    graphics_reset();
    memset(graphics.frame_buffer, 0xFF, sizeof(graphics.frame_buffer));

    Uint32 win_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI |
                       SDL_WINDOW_BORDERLESS;
#if WINDOW_RESIZABLE
    win_flags |= SDL_WINDOW_RESIZABLE;
#endif
#if WINDOW_FULLSCREEN
    win_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif

    int default_w, default_h;
    ui_default_window_size(&default_w, &default_h);

    graphics.window = SDL_CreateWindow(
        EMU_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        default_w,
        default_h,
        win_flags
    );

    if (!graphics.window) {
        LOG_ERROR(LOG_GRAPHICS, "MAIN WINDOW CREATE FAILED: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    // SET ICON. SDL COPIES THE SURFACE, SO WE FREE IT IMMEDIATELY.
    SDL_Surface* icon = make_app_icon();
    if (icon) {
        SDL_SetWindowIcon(graphics.window, icon);
        SDL_FreeSurface(icon);
    }
    int min_w, min_h;
    ui_min_window_size(&min_w, &min_h);
    SDL_SetWindowMinimumSize(graphics.window, min_w, min_h);

    graphics_update_title_for_cart();

    Uint32 rend_flags = SDL_RENDERER_ACCELERATED;
#if WINDOW_VSYNC
    rend_flags |= SDL_RENDERER_PRESENTVSYNC;
#endif
    graphics.renderer = SDL_CreateRenderer(graphics.window, -1, rend_flags);

    if (!graphics.renderer) {
        LOG_ERROR(LOG_GRAPHICS, "MAIN RENDERER CREATE FAILED: %s\n", SDL_GetError());
        SDL_DestroyWindow(graphics.window);
        SDL_Quit();
        exit(1);
    }

    graphics.texture = SDL_CreateTexture(graphics.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!graphics.texture) {
        LOG_ERROR(LOG_GRAPHICS, "MAIN TEXTURE CREATE FAILED: %s\n", SDL_GetError());
        SDL_DestroyRenderer(graphics.renderer);
        SDL_DestroyWindow(graphics.window);
        SDL_Quit();
        exit(1);
    }

    // CLEAR LETTERBOX BARS TO BLACK SO THEY DON'T FLICKER WITH STALE PIXELS
    SDL_SetRenderDrawColor(graphics.renderer, 0, 0, 0, 255);

    // INIT UI - OWNS THE MENU BAR AND PANELS
    ui_init(graphics.window, graphics.renderer);

    SDL_SetWindowHitTest(graphics.window, sdl_hit_test_cb, NULL);

#if DEBUG_WINDOW
    // DEBUG WINDOW SETUP - TILE ATLAS VIEWER
    int main_x, main_y, main_w, main_h;
    SDL_GetWindowPosition(graphics.window, &main_x, &main_y);
    SDL_GetWindowSize(graphics.window, &main_w, &main_h);

    graphics.debug_window = SDL_CreateWindow(
        "GBEmu - Tile Atlas",
        main_x + main_w + 10,
        main_y,
        256 * 2,
        256 * 2,
        SDL_WINDOW_SHOWN
    );

    if (!graphics.debug_window) {
        LOG_ERROR(LOG_GRAPHICS, "DEBUG WINDOW CREATE FAILED: %s\n", SDL_GetError());
        SDL_DestroyTexture(graphics.texture);
        SDL_DestroyRenderer(graphics.renderer);
        SDL_DestroyWindow(graphics.window);
        SDL_Quit();
        exit(1);
    }

    graphics.debug_renderer = SDL_CreateRenderer(graphics.debug_window, -1,
        SDL_RENDERER_ACCELERATED);

    if (!graphics.debug_renderer) {
        LOG_ERROR(LOG_GRAPHICS, "DEBUG RENDERER CREATE FAILED: %s\n", SDL_GetError());
        SDL_DestroyWindow(graphics.debug_window);
        SDL_DestroyTexture(graphics.texture);
        SDL_DestroyRenderer(graphics.renderer);
        SDL_DestroyWindow(graphics.window);
        SDL_Quit();
        exit(1);
    }

    SDL_RenderSetScale(graphics.debug_renderer, 2, 2);

    graphics.debug_texture = SDL_CreateTexture(graphics.debug_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        256, 256);

    if (!graphics.debug_texture) {
        LOG_ERROR(LOG_GRAPHICS, "DEBUG TEXTURE CREATE FAILED: %s\n", SDL_GetError());
        SDL_DestroyRenderer(graphics.debug_renderer);
        SDL_DestroyWindow(graphics.debug_window);
        SDL_DestroyTexture(graphics.texture);
        SDL_DestroyRenderer(graphics.renderer);
        SDL_DestroyWindow(graphics.window);
        SDL_Quit();
        exit(1);
    }
#endif

    LOG_INFO(LOG_GRAPHICS, "GRAPHICS INIT COMPLETE - VIDEO MODE: %s\n", SDL_GetCurrentVideoDriver());
    dump_frame_buffer_sample();
}

void graphics_tick() {
    if (!(graphics.lcdc & LCDC_ENABLE)) {
        // FILL FRAME BUFFER WITH WHITE IF WE HAVEN'T ALREADY
        static bool cleared = false;
        if (!cleared) {
            const u32 white = graphics.bg_colors[0];
            for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
                graphics.frame_buffer[i] = white;
            }
            cleared = true;
        }
        // RESET PPU STATE
        graphics.line = 0;
        graphics.ly = 0;
        graphics.window_line = 0;
        graphics.mode = MODE_HBLANK;
        graphics.stat &= ~0x03;
        graphics.mode_clock = 0;
        return;
    }

    // ADD 4 T-CYCLES
    graphics.mode_clock += 4;

    // LYC=LY CHECK
    if (graphics.ly == graphics.lyc) {
        graphics.stat |= 0x04;
        if (graphics.stat & 0x40) interrupt_req(INT_LCD);
    } else {
        graphics.stat &= ~0x04;
    }

    // VBLANK HANDLING
    if (graphics.mode == MODE_VBLANK) {
        if (graphics.mode_clock >= 456) {
            graphics.mode_clock = 0;
            graphics.line++;
            graphics.ly = graphics.line;

            // END OF VBLANK
            if (graphics.line > 153) {
                graphics.mode = MODE_OAM;
                graphics.line = 0;
                graphics.ly = 0;
                graphics.window_line = 0;
                graphics.stat = (graphics.stat & ~0x03) | MODE_OAM;
                if (graphics.stat & 0x20) interrupt_req(INT_LCD);  // MODE 2 STAT
            }
        }
        return;
    }

    // MODE TRANSITIONS
    const u16 OAM_END = 80;
    const u16 PIXEL_END = 80 + 168;
    const u16 LINE_END = 456;

    if (graphics.mode_clock < OAM_END) {
        if (graphics.mode != MODE_OAM) {
            graphics.mode = MODE_OAM;
            graphics.stat = (graphics.stat & ~0x03) | MODE_OAM;
            if (graphics.stat & 0x20) interrupt_req(INT_LCD);  // MODE 2 STAT
        }
    }
    else if (graphics.mode_clock < PIXEL_END) {
        if (graphics.mode != MODE_DRAWING) {
            graphics.mode = MODE_DRAWING;
            graphics.stat = (graphics.stat & ~0x03) | MODE_DRAWING;
        }
    }
    else if (graphics.mode_clock < LINE_END) {
        if (graphics.mode != MODE_HBLANK) {
            graphics.mode = MODE_HBLANK;
            graphics.stat = (graphics.stat & ~0x03) | MODE_HBLANK;
            render_line();
            if (graphics.stat & 0x08) interrupt_req(INT_LCD);
        }
    }
    else {
        graphics.mode_clock = 0;
        graphics.line++;
        graphics.ly = graphics.line;

        if (graphics.line == 144) {
            graphics.mode = MODE_VBLANK;
            graphics.stat = (graphics.stat & ~0x03) | MODE_VBLANK;
            interrupt_req(INT_VBLANK);
            if (graphics.stat & 0x10) interrupt_req(INT_LCD);
            draw_frame();
        }
    }
}

static gb_button key_to_button(SDL_Keycode k) {
    if (k == SDLK_UNKNOWN) return (gb_button)0;

    if (k == KEY_GB_RIGHT_PRI  || k == KEY_GB_RIGHT_ALT)  return BTN_RIGHT;
    if (k == KEY_GB_LEFT_PRI   || k == KEY_GB_LEFT_ALT)   return BTN_LEFT;
    if (k == KEY_GB_UP_PRI     || k == KEY_GB_UP_ALT)     return BTN_UP;
    if (k == KEY_GB_DOWN_PRI   || k == KEY_GB_DOWN_ALT)   return BTN_DOWN;
    if (k == KEY_GB_A_PRI      || k == KEY_GB_A_ALT)      return BTN_A;
    if (k == KEY_GB_B_PRI      || k == KEY_GB_B_ALT)      return BTN_B;
    if (k == KEY_GB_SELECT_PRI || k == KEY_GB_SELECT_ALT) return BTN_SELECT;
    if (k == KEY_GB_START_PRI  || k == KEY_GB_START_ALT)  return BTN_START;
    return (gb_button)0;
}

void handle_events() {
    ui_input_begin();
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        // GIVE UI FIRST CRACK. IF IT CONSUMES THE EVENT, SKIP EMU HANDLING.
        if (ui_handle_event(&event)) continue;

        if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
            && ui_wants_keyboard()
            && event.key.keysym.sym != KEY_GB_QUIT
            && event.key.keysym.sym != KEY_TOGGLE_FULLSCREEN) {
            continue;
        }

        switch (event.type)
        {
            case SDL_QUIT:
                get_gb()->die = true;
                break;

            case SDL_KEYDOWN:
                if (event.key.repeat) break;  // IGNORE OS KEY-REPEAT
                if (event.key.keysym.sym == KEY_GB_QUIT) {
                    get_gb()->die = true;
                    break;
                }
                if (event.key.keysym.sym == KEY_TOGGLE_FULLSCREEN) {
                    // TOGGLE BORDERLESS FULLSCREEN
                    Uint32 flags = SDL_GetWindowFlags(graphics.window);
                    bool is_fs = (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
                    SDL_SetWindowFullscreen(graphics.window,
                        is_fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                    break;
                }
                {
                    gb_button b = key_to_button(event.key.keysym.sym);
                    if (b) joypad_press(b);
                }
                break;

            case SDL_KEYUP:
                {
                    gb_button b = key_to_button(event.key.keysym.sym);
                    if (b) joypad_release(b);
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
#if DEBUG_WINDOW
                    // CLOSING THE DEBUG WINDOW JUST HIDES IT
                    if (event.window.windowID == SDL_GetWindowID(graphics.debug_window)) {
                        SDL_HideWindow(graphics.debug_window);
                    } else
#endif
                    if (event.window.windowID == SDL_GetWindowID(graphics.window)) {
                        get_gb()->die = true;
                    }
                }
                break;

            default:
                break;
        }
    }
    ui_input_end();
}

// MEMORY ACCESS
u8 vram_read(u16 addr) {
    return graphics.vram[addr - VRAM_START];
}

void vram_write(u16 addr, u8 val) {
    graphics.vram[addr - VRAM_START] = val;
}

u8 oam_read(u16 addr) {
    return ((u8*)graphics.oam)[addr - OAM_START];
}

void oam_write(u16 addr, u8 val) {
    // DITTO, NO BLOCKING
    ((u8*)graphics.oam)[addr - OAM_START] = val;
}

u8 lcd_read(u16 addr) {
   switch (addr - IO_START) {
       case IO_LCDC: return graphics.lcdc;
       case IO_STAT: return graphics.stat | 0x80;  // BIT 7 ALWAYS SET
       case IO_SCY: return graphics.scy;
       case IO_SCX: return graphics.scx;
       case IO_LY: return graphics.ly; // SET 0x90 FOR TESTS (OFF-BY-TWO ERR IS BULLSHIT)
       case IO_LYC: return graphics.lyc;
       case IO_DMA: return graphics.dma;
       case IO_BGP: return graphics.bgp;
       case IO_OBP0: return graphics.obp0;
       case IO_OBP1: return graphics.obp1;
       case IO_WY: return graphics.wy;
       case IO_WX: return graphics.wx;
       default: return 0xFF;
   }
}

void lcd_write(u16 addr, u8 val) {
    switch (addr - IO_START) {
        case IO_LCDC: {
            graphics.lcdc = val;
            if (!(val & LCDC_ENABLE)) {
                graphics.line = 0;
                graphics.ly = 0;
                graphics.window_line = 0;
                graphics.mode = MODE_HBLANK;
                graphics.mode_clock = 0;
                graphics.stat &= ~0x03;
                const u32 white = graphics.bg_colors[0];
                for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
                    graphics.frame_buffer[i] = white;
                }
            }
            break;
        }

        case IO_STAT:
            // KEEP MODE FLAGS, LY=LYC BIT, ONLY ALLOW WRITES TO INTERRUPT ENABLE BITS
            graphics.stat = (graphics.stat & 0x07) | (val & 0x78);
            break;

        case IO_SCY:
            graphics.scy = val;
            LOG_DEBUG(LOG_GRAPHICS, "SCY SET TO 0x%02X\n", val);
            break;

        case IO_SCX:
            graphics.scx = val;
            LOG_DEBUG(LOG_GRAPHICS, "SCX SET TO 0x%02X\n", val);
            break;

        case IO_LY:
            // READ ONLY
            break;

        case IO_LYC:
            graphics.lyc = val;
            if (graphics.ly == graphics.lyc) {
                graphics.stat |= 0x04;
                if (graphics.stat & 0x40) {
                    interrupt_req(INT_LCD);
                }
            } else {
                graphics.stat &= ~0x04;
            }
            LOG_DEBUG(LOG_GRAPHICS, "LYC SET TO 0x%02X\n", val);
            break;

        case IO_DMA:
            graphics.dma = val;
            dma_start(val);
            break;

        case IO_BGP:
            update_palette(&graphics.bgp, graphics.bg_colors, val);
            break;

        case IO_OBP0:
            update_palette(&graphics.obp0, graphics.sprite_colors[0], val);
            break;

        case IO_OBP1:
            update_palette(&graphics.obp1, graphics.sprite_colors[1], val);
            break;

        case IO_WY:
            graphics.wy = val;
            break;

        case IO_WX:
            graphics.wx = val;
            break;
    }
}

void update_debug_window() {
#if !DEBUG_WINDOW
    return;
#else
    static u32 debug_buffer[256 * 256];
    static int frame_count = 0;
    frame_count++;

    for (int i = 0; i < 256 * 256; i++) {
        debug_buffer[i] = 0xFF333333;
    }

    for (int ty = 0; ty < 32; ty++) {
        for (int tx = 0; tx < 16; tx++) {
            int base_x = tx * 8;
            int base_y = ty * 8;

            u32 border_color = ((tx + ty) % 2) ? 0xFF666666 : 0xFF444444;

            for (int x = 0; x < 8; x++) {
                debug_buffer[base_y * 256 + base_x + x] = border_color;
                debug_buffer[(base_y + 7) * 256 + base_x + x] = border_color;
            }

            for (int y = 0; y < 8; y++) {
                debug_buffer[(base_y + y) * 256 + base_x] = border_color;
                debug_buffer[(base_y + y) * 256 + base_x + 7] = border_color;
            }

            int tile_idx = ty * 16 + tx;
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    u8 color_idx = get_tile_pixel(tile_idx, x, y);
                    debug_buffer[(base_y + y) * 256 + base_x + x] = graphics.bg_colors[color_idx];
                }
            }
        }
    }

    SDL_UpdateTexture(graphics.debug_texture, NULL, debug_buffer, 256 * sizeof(u32));
    SDL_RenderClear(graphics.debug_renderer);
    SDL_RenderCopy(graphics.debug_renderer, graphics.debug_texture, NULL, NULL);
    SDL_RenderPresent(graphics.debug_renderer);

    if (frame_count % 60 == 0) {
        LOG_TRACE(LOG_GRAPHICS, "Updated debug window frame %d\n", frame_count);
    }
#endif
}

void dump_frame_buffer_sample() {
    LOG_TRACE(LOG_GRAPHICS, "Frame Buffer Sample (first 16 pixels):\n");
    for (int i = 0; i < 16; i++) {
        LOG_TRACE(LOG_GRAPHICS, "%08X ", graphics.frame_buffer[i]);
        if ((i + 1) % 4 == 0) LOG_TRACE(LOG_GRAPHICS, "\n");
    }
}

#include <stddef.h>

void graphics_reset(void) {
    const size_t saveable = offsetof(graphics_system, window);
    memset(&graphics, 0, saveable);
    graphics.mode = MODE_HBLANK;

    if (get_bootrom_enable()) {
    } else {
        graphics.lcdc = 0x91;
        graphics.stat = 0x85;
        graphics.bgp  = 0xFC;
        graphics.obp0 = 0xFF;
        graphics.obp1 = 0xFF;
    }

    update_palette(&graphics.bgp,  graphics.bg_colors,        graphics.bgp);
    update_palette(&graphics.obp0, graphics.sprite_colors[0], graphics.obp0);
    update_palette(&graphics.obp1, graphics.sprite_colors[1], graphics.obp1);
}

void graphics_save_state(FILE* fp) {
    const size_t saveable = offsetof(graphics_system, window);
    fwrite(&graphics, saveable, 1, fp);
}
void graphics_load_state(FILE* fp) {
    const size_t saveable = offsetof(graphics_system, window);
    fread(&graphics, saveable, 1, fp);
}
