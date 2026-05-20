#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

#include "ui.h"
#include "gameboy.h"
#include "cpu.h"
#include "apu.h"
#include "cart.h"
#include "bus.h"

#include <SDL2/SDL.h>
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MENU_H            28
#define MENU_BEVEL         2
#define RESIZE_BORDER      6
#define DRAG_CORNER_SZ    16
#define WIN_BTN_W         30

#define MENUBAR_PAD_X      4
#define MENUBAR_PAD_Y      0
#define MENUBAR_SPACING    2

#define PANEL_PAD          8
#define PANEL_SPACING      4
#define ROW_H_LBL         18
#define ROW_H_CTL         24
#define ROW_H_HEX         16
#define BORDER_W           2

#define DEFAULT_GB_SCALE   5
#define DEFAULT_PAD_X     20
#define DEFAULT_PAD_BOT   28

// MENU LAYOUT TABLE
typedef struct { const char* label; float width; } MenuSpec;
static const MenuSpec MENUS[] = {
    {"File",   46},
    {"Run",    44},
    {"View",   46},
    {"Audio",  56},
    {"Debug",  56},
    {"Theme",  56},
    {"Help",   46},
};
#define MENU_COUNT ((int)(sizeof(MENUS) / sizeof(MENUS[0])))

// EACH MENU OWN WINDOW NAME ELSE SIZE BLEEDS
static const char* const DROPDOWN_NAMES[MENU_COUNT] = {
    "_dd_file", "_dd_run", "_dd_view", "_dd_audio",
    "_dd_debug", "_dd_theme", "_dd_help",
};

static float menus_total_w(void) {
    float w = 0; for (int i = 0; i < MENU_COUNT; i++) w += MENUS[i].width;
    return w;
}
// THEMES

typedef struct {
    const char* name;
    struct nk_color shell;
    struct nk_color shell_hi;
    struct nk_color shell_lo;
    struct nk_color bezel;
    struct nk_color screen;
    struct nk_color accent;
    struct nk_color text;
    struct nk_color text_alt;
    struct nk_color screen_text;
    struct nk_color screen_dim;
} Theme;

#define C(r,g,b) {(r),(g),(b),255}

static const Theme THEMES[] = {
    {"DMG",
     C(0xa8,0xa9,0x9a), C(0xc4,0xc4,0xb4), C(0x5a,0x5a,0x50),
     C(0x18,0x18,0x10), C(0x9b,0xbc,0x0f), C(0xc0,0x40,0x70),
     C(0x18,0x18,0x18), C(0x4e,0x4e,0x46),
     C(0x0f,0x38,0x0f), C(0x30,0x62,0x30)},
    {"Atomic",
     C(0x8c,0x68,0xc8), C(0xb4,0x92,0xe0), C(0x40,0x24,0x6c),
     C(0x12,0x08,0x24), C(0xc8,0xae,0xe8), C(0xe6,0x4a,0x80),
     C(0xf4,0xee,0xff), C(0xd6,0xc4,0xee),
     C(0x22,0x10,0x4a), C(0x52,0x36,0x86)},
    {"Berry",
     C(0xd8,0x4a,0x84), C(0xee,0x80,0xa8), C(0x6a,0x18,0x40),
     C(0x22,0x08,0x18), C(0xff,0xc8,0xd8), C(0xc0,0x90,0x18),
     C(0xff,0xff,0xff), C(0xff,0xc0,0xd0),
     C(0x4a,0x0c,0x28), C(0x82,0x2a,0x52)},
    {"Grape",
     C(0x5a,0x3c,0xae), C(0x82,0x66,0xd6), C(0x22,0x14,0x60),
     C(0x0c,0x04,0x24), C(0xc8,0xb4,0xf0), C(0xff,0xc4,0x40),
     C(0xff,0xff,0xff), C(0xc8,0xb4,0xf0),
     C(0x22,0x14,0x60), C(0x4a,0x32,0x96)},
    {"Dandelion",
     C(0xf6,0xc6,0x1c), C(0xff,0xe2,0x6a), C(0x8a,0x5c,0x00),
     C(0x2a,0x18,0x00), C(0xff,0xee,0xa0), C(0xb6,0x36,0x0a),
     C(0x2c,0x1c,0x00), C(0x70,0x4a,0x00),
     C(0x4a,0x30,0x00), C(0x80,0x56,0x00)},
    {"Kiwi",
     C(0x82,0xc6,0x28), C(0xb6,0xe0,0x5a), C(0x3a,0x60,0x08),
     C(0x10,0x20,0x00), C(0xdc,0xf6,0x96), C(0xc8,0x46,0x10),
     C(0x14,0x2c,0x00), C(0x3a,0x60,0x08),
     C(0x14,0x30,0x06), C(0x3e,0x66,0x18)},
    {"Mint",
     C(0xb0,0xe8,0xc8), C(0xd6,0xf6,0xe2), C(0x40,0x80,0x66),
     C(0x10,0x2a,0x1e), C(0xe6,0xfa,0xea), C(0xb0,0x36,0x4a),
     C(0x08,0x30,0x20), C(0x2e,0x66,0x4a),
     C(0x06,0x36,0x22), C(0x2a,0x60,0x4a)},
    {"Silver",
     C(0xc2,0xc2,0xca), C(0xe2,0xe2,0xea), C(0x55,0x55,0x5e),
     C(0x18,0x18,0x1e), C(0xee,0xee,0xf2), C(0x8a,0x18,0x36),
     C(0x14,0x14,0x1c), C(0x44,0x44,0x4e),
     C(0x16,0x16,0x1c), C(0x4a,0x4a,0x52)},
    {"Teal",
     C(0x14,0xb0,0xa6), C(0x46,0xd2,0xc8), C(0x00,0x54,0x54),
     C(0x00,0x1c,0x1c), C(0xb8,0xec,0xe6), C(0xff,0xc0,0x40),
     C(0xff,0xff,0xff), C(0xb8,0xec,0xe6),
     C(0x00,0x2a,0x2a), C(0x14,0x5a,0x52)},
    {"Pine",
     C(0x16,0x6e,0x6e), C(0x40,0x96,0x90), C(0x02,0x2e,0x2c),
     C(0x00,0x10,0x10), C(0x88,0xc4,0xc0), C(0xff,0xb8,0x40),
     C(0xff,0xff,0xff), C(0x88,0xc4,0xc0),
     C(0x00,0x22,0x22), C(0x14,0x4a,0x46)},
    {"Cosmo",
     C(0x4c,0x36,0x86), C(0x76,0x5e,0xb8), C(0x1c,0x10,0x3c),
     C(0x06,0x02,0x18), C(0xb8,0xa0,0xdc), C(0xff,0xd2,0x4a),
     C(0xff,0xff,0xff), C(0xc8,0xb4,0xea),
     C(0x1a,0x0a,0x42), C(0x42,0x2a,0x80)},
    {"Charcoal",
     C(0x36,0x36,0x3e), C(0x55,0x55,0x60), C(0x12,0x12,0x18),
     C(0x00,0x00,0x06), C(0x6c,0x6c,0x74), C(0xff,0x40,0x40),
     C(0xee,0xee,0xf2), C(0xb4,0xb4,0xbc),
     C(0xee,0xee,0xf2), C(0xb4,0xb4,0xbc)},
    {"Pikachu",
     C(0xfd,0xd9,0x1c), C(0xff,0xee,0x60), C(0xa6,0x60,0x00),
     C(0x30,0x18,0x00), C(0xff,0xf0,0xa6), C(0xa6,0x2e,0x14),
     C(0x2c,0x18,0x00), C(0x80,0x4c,0x00),
     C(0x46,0x2a,0x00), C(0x82,0x52,0x00)},
    {"Silvery",
     C(0xa6,0xa8,0xb2), C(0xcc,0xce,0xd6), C(0x4e,0x4e,0x58),
     C(0x16,0x16,0x1c), C(0xe2,0xe4,0xea), C(0x82,0x36,0x18),
     C(0x10,0x10,0x1a), C(0x42,0x42,0x4c),
     C(0x10,0x10,0x1a), C(0x44,0x44,0x52)},
    {"Lagoon",
     C(0xb6,0xe8,0xd2), C(0xd8,0xf6,0xe6), C(0x42,0x80,0x6e),
     C(0x0c,0x28,0x1e), C(0xe2,0xfa,0xee), C(0xc8,0x6c,0x10),
     C(0x10,0x36,0x2a), C(0x36,0x6c,0x5a),
     C(0x06,0x36,0x26), C(0x2e,0x66,0x52)},
    {"Tangerine",
     C(0xf6,0x82,0x1c), C(0xff,0xae,0x5a), C(0x8c,0x3c,0x00),
     C(0x28,0x10,0x00), C(0xff,0xd2,0xa6), C(0x1a,0x5a,0xc8),
     C(0x40,0x18,0x00), C(0x80,0x36,0x00),
     C(0x46,0x1c,0x00), C(0x82,0x36,0x00)},
};
#define THEME_COUNT ((int)(sizeof(THEMES) / sizeof(THEMES[0])))

// STATE

static struct nk_context* ctx = NULL;
static SDL_Window*   g_window   = NULL;
static SDL_Renderer* g_renderer = NULL;
static bool ui_initialized = false;

static int active_theme = 0;

static int active_menu = -1;
static struct nk_rect menu_header_bounds[MENU_COUNT];
static struct nk_rect dropdown_bounds;
static bool menubar_visible_last_frame = true;

static bool show_cpu   = false;
static bool show_mem   = false;
static bool show_audio = false;
static bool show_file  = false;
static bool show_about = false;
static bool show_theme = false;

// RESET BOUNDS ON OPEN ELSE PANEL WONT REOPEN
static bool reset_cpu_bounds   = true;
static bool reset_mem_bounds   = true;
static bool reset_audio_bounds = true;
static bool reset_file_bounds  = true;
static bool reset_about_bounds = true;
static bool reset_theme_bounds = true;

#define MAX_FILES     512
#define MAX_PATH_LEN 1024
typedef struct {
    char name[256];
    bool is_dir;
} BrowserEntry;
static char         browser_dir[MAX_PATH_LEN] = "roms";
static BrowserEntry browser_entries[MAX_FILES];
static int          browser_count    = 0;
static int          browser_selected = -1;
static bool         browser_dirty    = true;

static char mem_addr_buf[8] = "C000";
static int  mem_addr_buf_len = 4;

// HELPERS

static void win_size(int* w, int* h) {
    *w = 800; *h = 600;
    if (g_window) SDL_GetWindowSize(g_window, w, h);
}

static const Theme* T(void) { return &THEMES[active_theme]; }

static struct nk_rect panel_rect(float x, float y, float w, float h) {
    int win_w, win_h;
    win_size(&win_w, &win_h);
    if (x + w > win_w) x = (float)win_w - w;
    if (y + h > win_h) y = (float)win_h - h;
    if (x < 0)                  x = 0;
    if (y < (float)MENU_H)      y = (float)MENU_H;
    return nk_rect(x, y, w, h);
}

static bool window_is_fullscreen(void) {
    if (!g_window) return false;
    Uint32 flags = SDL_GetWindowFlags(g_window);
    return (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
}

static bool menubar_should_show(void) {
    if (!window_is_fullscreen()) return true;
    if (active_menu != -1)       return true;
    if (show_cpu || show_mem || show_audio || show_file || show_about || show_theme) return true;
    if (!ctx) return false;
    float mouse_y = ctx->input.mouse.pos.y;
    float threshold = menubar_visible_last_frame ? (float)(MENU_H * 2)
                                                 : (float)MENU_H;
    return mouse_y >= 0.0f && mouse_y < threshold;
}

static void panel_open(const char* name, bool* visible, bool* reset_flag) {
    if (ctx) {
        nk_window_show(ctx, name, NK_SHOWN);
    }
    *visible = true;
    *reset_flag = true;
}

static void cycle_theme(int delta) {
    active_theme = (active_theme + delta + THEME_COUNT) % THEME_COUNT;
}

static void apply_theme(void) {
    const Theme* th = T();
    struct nk_style* s = &ctx->style;

    struct nk_color shell      = th->shell;
    struct nk_color shell_lo   = th->shell_lo;
    struct nk_color bezel      = th->bezel;
    struct nk_color screen     = th->screen;
    struct nk_color accent     = th->accent;
    struct nk_color text       = th->text;
    struct nk_color text_alt   = th->text_alt;

    struct nk_color accent_hi = {
        (nk_byte)((accent.r + 0x30 < 255) ? accent.r + 0x30 : 255),
        (nk_byte)((accent.g + 0x30 < 255) ? accent.g + 0x30 : 255),
        (nk_byte)((accent.b + 0x30 < 255) ? accent.b + 0x30 : 255), 255};
    struct nk_color accent_lo = {
        (nk_byte)((accent.r > 0x28) ? accent.r - 0x28 : 0),
        (nk_byte)((accent.g > 0x28) ? accent.g - 0x28 : 0),
        (nk_byte)((accent.b > 0x28) ? accent.b - 0x28 : 0), 255};

    s->text.color = text;

    s->window.background           = shell;
    s->window.fixed_background     = nk_style_item_color(shell);
    s->window.border_color         = shell_lo;
    s->window.popup_border_color   = shell_lo;
    s->window.combo_border_color   = shell_lo;
    s->window.contextual_border_color = shell_lo;
    s->window.menu_border_color    = shell_lo;
    s->window.group_border_color   = shell_lo;
    s->window.tooltip_border_color = shell_lo;
    s->window.scaler               = nk_style_item_color(accent);
    s->window.border               = BORDER_W;
    s->window.combo_border         = BORDER_W;
    s->window.contextual_border    = BORDER_W;
    s->window.menu_border          = BORDER_W;
    s->window.group_border         = BORDER_W;
    s->window.tooltip_border       = BORDER_W;
    s->window.popup_border         = BORDER_W;
    s->window.rounding             = 0.0f;
    s->window.padding              = nk_vec2(PANEL_PAD, PANEL_PAD);
    s->window.spacing              = nk_vec2(PANEL_SPACING, PANEL_SPACING);
    s->window.group_padding        = nk_vec2(PANEL_PAD/2, PANEL_PAD/2);
    s->window.popup_padding        = nk_vec2(PANEL_PAD/2, PANEL_PAD/2);
    s->window.combo_padding        = nk_vec2(4, 4);
    s->window.contextual_padding   = nk_vec2(4, 4);
    s->window.menu_padding         = nk_vec2(4, 4);
    s->window.tooltip_padding      = nk_vec2(4, 4);
    s->window.min_row_height_padding = 4;

    s->window.header.normal = nk_style_item_color(bezel);
    s->window.header.hover  = nk_style_item_color(bezel);
    s->window.header.active = nk_style_item_color(bezel);
    s->window.header.label_normal  = (struct nk_color){0xff,0xff,0xff,0xff};
    s->window.header.label_hover   = (struct nk_color){0xff,0xff,0xff,0xff};
    s->window.header.label_active  = (struct nk_color){0xff,0xff,0xff,0xff};
    s->window.header.padding       = nk_vec2(PANEL_PAD/2, 3);
    s->window.header.label_padding = nk_vec2(2, 2);
    s->window.header.spacing       = nk_vec2(4, 2);
    s->window.header.align         = NK_HEADER_LEFT;
    s->window.header.close_symbol  = NK_SYMBOL_X;
    s->window.header.minimize_symbol  = NK_SYMBOL_MINUS;
    s->window.header.maximize_symbol  = NK_SYMBOL_PLUS;

    s->window.header.close_button.normal  = nk_style_item_color(bezel);
    s->window.header.close_button.hover   = nk_style_item_color((struct nk_color){0xc8,0x30,0x30,0xff});
    s->window.header.close_button.active  = nk_style_item_color((struct nk_color){0x80,0x20,0x20,0xff});
    s->window.header.close_button.border_color  = shell_lo;
    s->window.header.close_button.text_background = bezel;
    s->window.header.close_button.text_normal = (struct nk_color){0xff,0xff,0xff,0xff};
    s->window.header.close_button.text_hover  = (struct nk_color){0xff,0xff,0xff,0xff};
    s->window.header.close_button.text_active = (struct nk_color){0xff,0xff,0xff,0xff};
    s->window.header.close_button.text_alignment = NK_TEXT_CENTERED;
    s->window.header.close_button.border = 0;
    s->window.header.close_button.rounding = 0;
    s->window.header.close_button.padding = nk_vec2(5, 5);
    s->window.header.close_button.touch_padding = nk_vec2(3, 3);
    s->window.header.minimize_button = s->window.header.close_button;
    s->window.header.minimize_button.hover = nk_style_item_color(accent);
    s->window.header.minimize_button.active = nk_style_item_color(accent_lo);

    s->button.normal       = nk_style_item_color(accent);
    s->button.hover        = nk_style_item_color(accent_hi);
    s->button.active       = nk_style_item_color(accent_lo);
    s->button.border_color = shell_lo;
    s->button.text_background = accent;
    s->button.text_normal  = (struct nk_color){0xff,0xff,0xff,0xff};
    s->button.text_hover   = (struct nk_color){0xff,0xff,0xff,0xff};
    s->button.text_active  = (struct nk_color){0xff,0xff,0xff,0xff};
    s->button.text_alignment = NK_TEXT_CENTERED;
    s->button.border       = BORDER_W;
    s->button.rounding     = 0;
    s->button.padding      = nk_vec2(4, 2);
    s->button.image_padding = nk_vec2(0, 0);
    s->button.touch_padding = nk_vec2(0, 0);

    s->contextual_button = s->button;
    s->contextual_button.normal = nk_style_item_color(screen);
    s->contextual_button.hover  = nk_style_item_color(accent);
    s->contextual_button.active = nk_style_item_color(accent_lo);
    s->contextual_button.text_normal = text;
    s->contextual_button.text_hover  = (struct nk_color){0xff,0xff,0xff,0xff};
    s->contextual_button.text_active = (struct nk_color){0xff,0xff,0xff,0xff};
    s->contextual_button.text_alignment = NK_TEXT_LEFT;
    s->contextual_button.border = 0;
    s->contextual_button.padding = nk_vec2(8, 3);

    s->menu_button = s->button;
    s->menu_button.normal = nk_style_item_color(shell);
    s->menu_button.hover  = nk_style_item_color(accent_hi);
    s->menu_button.active = nk_style_item_color(accent);
    s->menu_button.text_normal = text;
    s->menu_button.text_hover  = (struct nk_color){0xff,0xff,0xff,0xff};
    s->menu_button.text_active = (struct nk_color){0xff,0xff,0xff,0xff};
    s->menu_button.border = 0;
    s->menu_button.padding = nk_vec2(6, 2);

    s->checkbox.normal        = nk_style_item_color(screen);
    s->checkbox.hover         = nk_style_item_color(screen);
    s->checkbox.active        = nk_style_item_color(screen);
    s->checkbox.cursor_normal = nk_style_item_color(accent);
    s->checkbox.cursor_hover  = nk_style_item_color(accent_hi);
    s->checkbox.text_normal   = text;
    s->checkbox.text_hover    = text;
    s->checkbox.text_active   = text;
    s->checkbox.text_background = shell;
    s->checkbox.border_color  = shell_lo;
    s->checkbox.border        = BORDER_W;
    s->checkbox.padding       = nk_vec2(2, 2);
    s->checkbox.spacing       = 4;

    // SELECTABLE USE SCREEN_TEXT ELSE INVISIBLE ON LIGHT THEMES
    s->selectable.normal             = nk_style_item_color(screen);
    s->selectable.hover              = nk_style_item_color(accent_hi);
    s->selectable.pressed            = nk_style_item_color(accent);
    s->selectable.normal_active      = nk_style_item_color(accent);
    s->selectable.hover_active       = nk_style_item_color(accent);
    s->selectable.pressed_active     = nk_style_item_color(accent_lo);
    s->selectable.text_normal        = th->screen_text;
    s->selectable.text_hover         = (struct nk_color){0xff,0xff,0xff,0xff};
    s->selectable.text_pressed       = (struct nk_color){0xff,0xff,0xff,0xff};
    s->selectable.text_normal_active = (struct nk_color){0xff,0xff,0xff,0xff};
    s->selectable.text_hover_active  = (struct nk_color){0xff,0xff,0xff,0xff};
    s->selectable.text_pressed_active = (struct nk_color){0xff,0xff,0xff,0xff};
    s->selectable.text_background    = screen;
    s->selectable.text_alignment     = NK_TEXT_LEFT;
    s->selectable.rounding           = 0;
    s->selectable.padding            = nk_vec2(4, 2);

    s->slider.normal       = nk_style_item_color(screen);
    s->slider.hover        = nk_style_item_color(screen);
    s->slider.active       = nk_style_item_color(screen);
    s->slider.bar_normal   = shell_lo;
    s->slider.bar_hover    = shell_lo;
    s->slider.bar_active   = shell_lo;
    s->slider.bar_filled   = accent;
    s->slider.cursor_normal = nk_style_item_color(accent);
    s->slider.cursor_hover  = nk_style_item_color(accent_hi);
    s->slider.cursor_active = nk_style_item_color(accent_lo);
    s->slider.border_color = shell_lo;
    s->slider.rounding     = 0;
    s->slider.bar_height   = 6;
    s->slider.padding      = nk_vec2(4, 4);
    s->slider.cursor_size  = nk_vec2(10, 14);

    s->edit.normal        = nk_style_item_color(screen);
    s->edit.hover         = nk_style_item_color(screen);
    s->edit.active        = nk_style_item_color(screen);
    s->edit.border_color  = shell_lo;
    s->edit.cursor_normal = text;
    s->edit.cursor_hover  = text;
    s->edit.cursor_text_normal = text;
    s->edit.cursor_text_hover  = text;
    s->edit.text_normal   = text;
    s->edit.text_hover    = text;
    s->edit.text_active   = text;
    s->edit.selected_normal = accent;
    s->edit.selected_hover  = accent;
    s->edit.selected_text_normal = (struct nk_color){0xff,0xff,0xff,0xff};
    s->edit.selected_text_hover  = (struct nk_color){0xff,0xff,0xff,0xff};
    s->edit.border       = BORDER_W;
    s->edit.rounding     = 0;
    s->edit.padding      = nk_vec2(4, 2);
    s->edit.row_padding  = 2;
    s->edit.cursor_size  = 1;

    s->scrollv.normal       = nk_style_item_color(shell_lo);
    s->scrollv.hover        = nk_style_item_color(shell_lo);
    s->scrollv.active       = nk_style_item_color(shell_lo);
    s->scrollv.cursor_normal = nk_style_item_color(accent);
    s->scrollv.cursor_hover  = nk_style_item_color(accent_hi);
    s->scrollv.cursor_active = nk_style_item_color(accent_lo);
    s->scrollv.border_color  = shell_lo;
    s->scrollv.cursor_border_color = shell_lo;
    s->scrollv.rounding      = 0;
    s->scrollv.rounding_cursor = 0;
    s->scrollv.border        = 0;
    s->scrollv.border_cursor = 0;
    s->scrollv.padding       = nk_vec2(0, 0);
    s->scrollh = s->scrollv;

    (void)text_alt;
}
// CHROME

static inline void sdl_setcolor(SDL_Renderer* r, struct nk_color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

#define CASE_SHELL    5
#define CASE_GROOVE   2
#define CASE_BEZEL    5
#define CASE_INSET    (CASE_SHELL + CASE_GROOVE + CASE_BEZEL)

#define GRAIN_TILE 128
static SDL_Texture* g_grain_tex   = NULL;
static int          g_grain_theme = -1;

static inline u32 hash_pixel(int x, int y, int salt) {
    u32 h = (u32)x * 0x9E3779B1u ^ (u32)y * 0x85EBCA77u ^ (u32)salt * 0xC2B2AE3Du;
    h ^= h >> 15; h *= 0x85EBCA6Bu;
    h ^= h >> 13; h *= 0xC2B2AE35u;
    h ^= h >> 16;
    return h;
}

static u32 pack_argb(struct nk_color c) {
    return 0xFF000000u | ((u32)c.r << 16) | ((u32)c.g << 8) | (u32)c.b;
}

static struct nk_color blend(struct nk_color a, struct nk_color b, int t) {
    if (t < 0) t = 0;
    if (t > 255) t = 255;
    struct nk_color r;
    r.r = (nk_byte)(((255 - t) * a.r + t * b.r) / 255);
    r.g = (nk_byte)(((255 - t) * a.g + t * b.g) / 255);
    r.b = (nk_byte)(((255 - t) * a.b + t * b.b) / 255);
    r.a = 255;
    return r;
}

static void ensure_grain_tex(SDL_Renderer* r) {
    if (g_grain_theme == active_theme && g_grain_tex) return;
    if (g_grain_tex) { SDL_DestroyTexture(g_grain_tex); g_grain_tex = NULL; }

    g_grain_tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STATIC, GRAIN_TILE, GRAIN_TILE);
    if (!g_grain_tex) return;

    const Theme* th = T();
    static u32 px[GRAIN_TILE * GRAIN_TILE];

    // GRAIN MUST BE PURE SPECKLE OR SEAMS SHOW
    for (int i = 0; i < GRAIN_TILE * GRAIN_TILE; i++) {
        int x = i % GRAIN_TILE;
        int y = i / GRAIN_TILE;
        u32 h = hash_pixel(x, y, active_theme);
        u32 v = h & 0xFF;
        struct nk_color c = th->shell;
        if (v < 12)       c = blend(c, th->shell_lo, 60);
        else if (v < 32)  c = blend(c, th->shell_lo, 24);
        else if (v < 60)  c = blend(c, th->shell_hi, 32);
        px[i] = pack_argb(c);
    }
    SDL_UpdateTexture(g_grain_tex, NULL, px, GRAIN_TILE * sizeof(u32));
    g_grain_theme = active_theme;
}

static void tile_grain(SDL_Renderer* r, int rx, int ry, int rw, int rh) {
    if (!g_grain_tex) return;
    for (int y = ry; y < ry + rh; y += GRAIN_TILE) {
        for (int x = rx; x < rx + rw; x += GRAIN_TILE) {
            SDL_Rect src = {0, 0, GRAIN_TILE, GRAIN_TILE};
            SDL_Rect dst = {x, y, GRAIN_TILE, GRAIN_TILE};
            if (dst.x + dst.w > rx + rw) {
                src.w = dst.w = (rx + rw) - dst.x;
            }
            if (dst.y + dst.h > ry + rh) {
                src.h = dst.h = (ry + rh) - dst.y;
            }
            SDL_RenderCopy(r, g_grain_tex, &src, &dst);
        }
    }
}

void ui_draw_chrome_under(SDL_Renderer* r, int win_w, int win_h) {
    if (!ui_initialized) return;
    if (window_is_fullscreen()) return;
    int menu_h = ui_menu_height();
    if (menu_h >= win_h) return;

    const Theme* th = T();
    ensure_grain_tex(r);

    sdl_setcolor(r, th->shell);
    SDL_Rect case_rect = {0, menu_h, win_w, win_h - menu_h};
    SDL_RenderFillRect(r, &case_rect);

    tile_grain(r, 0, menu_h, win_w, win_h - menu_h);

    SDL_Rect lcd = ui_screen_rect(win_w, win_h);

    sdl_setcolor(r, th->shell_lo);
    SDL_Rect groove = {
        lcd.x - CASE_BEZEL - CASE_GROOVE,
        lcd.y - CASE_BEZEL - CASE_GROOVE,
        lcd.w + 2 * (CASE_BEZEL + CASE_GROOVE),
        lcd.h + 2 * (CASE_BEZEL + CASE_GROOVE),
    };
    SDL_RenderFillRect(r, &groove);

    sdl_setcolor(r, th->bezel);
    SDL_Rect bz = {
        lcd.x - CASE_BEZEL,
        lcd.y - CASE_BEZEL,
        lcd.w + 2 * CASE_BEZEL,
        lcd.h + 2 * CASE_BEZEL,
    };
    SDL_RenderFillRect(r, &bz);

    sdl_setcolor(r, th->shell_hi);
    SDL_Rect bz_hi = {bz.x, bz.y - 1, bz.w, 1};
    SDL_RenderFillRect(r, &bz_hi);

}

static void draw_drag_grip(SDL_Renderer* r, int win_w, int win_h) {
    if (window_is_fullscreen()) return;
    const Theme* th = T();
    const int dot = 2;
    const int step = 4;
    const int pad = 3;
    const int bx = win_w - pad;
    const int by = win_h - pad;

    sdl_setcolor(r, th->shell_lo);
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col <= row; col++) {
            SDL_Rect px = {
                bx - (col + 1) * step + 1,
                by - (3 - row) * step + 1,
                dot, dot
            };
            SDL_RenderFillRect(r, &px);
        }
    }
    sdl_setcolor(r, th->shell_hi);
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col <= row; col++) {
            SDL_Rect px = {
                bx - (col + 1) * step,
                by - (3 - row) * step,
                dot, dot
            };
            SDL_RenderFillRect(r, &px);
        }
    }
}

void ui_draw_chrome_over(SDL_Renderer* r, int win_w, int win_h) {
    if (!ui_initialized) return;
    if (window_is_fullscreen()) return;
    int menu_h = ui_menu_height();
    if (menu_h <= 0) return;

    const Theme* th = T();

    sdl_setcolor(r, th->shell_hi);
    SDL_Rect top = {0, 0, win_w, MENU_BEVEL};
    SDL_RenderFillRect(r, &top);
    SDL_Rect sub = {0, MENU_BEVEL, win_w, 1};
    SDL_RenderFillRect(r, &sub);

    sdl_setcolor(r, th->shell_lo);
    SDL_Rect bot = {0, menu_h - MENU_BEVEL, win_w, MENU_BEVEL};
    SDL_RenderFillRect(r, &bot);

    draw_drag_grip(r, win_w, win_h);
}

SDL_Rect ui_screen_rect(int win_w, int win_h) {
    bool fs = window_is_fullscreen();
    int menu_h = fs ? 0 : MENU_H;
    int inset  = fs ? 0 : CASE_INSET;

    int avail_w = win_w - 2 * inset;
    int avail_h = win_h - menu_h - 2 * inset;
    if (avail_w < 1) avail_w = 1;
    if (avail_h < 1) avail_h = 1;

    int sx = avail_w / SCREEN_WIDTH;
    int sy = avail_h / SCREEN_HEIGHT;
    int scale = sx < sy ? sx : sy;

    SDL_Rect dst;
    if (scale >= 1) {
        dst.w = SCREEN_WIDTH * scale;
        dst.h = SCREEN_HEIGHT * scale;
    } else {
        float a_win = (float)avail_w / (float)avail_h;
        float a_gb  = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
        if (a_win > a_gb) {
            dst.h = avail_h;
            dst.w = (int)(avail_h * a_gb);
        } else {
            dst.w = avail_w;
            dst.h = (int)(avail_w / a_gb);
        }
    }
    dst.x = inset + (avail_w - dst.w) / 2;
    dst.y = menu_h + inset + (avail_h - dst.h) / 2;
    return dst;
}

void ui_default_window_size(int* w, int* h) {
    *w = SCREEN_WIDTH  * DEFAULT_GB_SCALE + 2 * CASE_INSET + 2 * DEFAULT_PAD_X;
    *h = SCREEN_HEIGHT * DEFAULT_GB_SCALE + MENU_H + 2 * CASE_INSET + DEFAULT_PAD_BOT;
}

void ui_min_window_size(int* w, int* h) {
    *w = SCREEN_WIDTH  + 2 * CASE_INSET;
    *h = SCREEN_HEIGHT + MENU_H + 2 * CASE_INSET;
}

// TYPOGRAPHY AND FONT STUFF

typedef struct { char ch; u8 rows[7]; } Glyph;

static const Glyph FONT[] = {
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
    {'6', {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}},
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06}},
    {',', {0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x04}},
    {'!', {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}},
    {'?', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}},
    {'-', {0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00}},
    {'+', {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}},
    {'/', {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10}},
    {':', {0x00, 0x06, 0x06, 0x00, 0x06, 0x06, 0x00}},
    {'(', {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}},
    {')', {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}},
    {'>', {0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10}},
    {'<', {0x01, 0x02, 0x04, 0x08, 0x04, 0x02, 0x01}},
    {'*', {0x00, 0x0A, 0x04, 0x1F, 0x04, 0x0A, 0x00}},
    {'~', {0x00, 0x00, 0x09, 0x16, 0x00, 0x00, 0x00}},
};
#define FONT_COUNT ((int)(sizeof(FONT) / sizeof(FONT[0])))

static const Glyph* find_glyph(char c) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    for (int i = 0; i < FONT_COUNT; i++) {
        if (FONT[i].ch == c) return &FONT[i];
    }
    return NULL;
}

static int text_w_px(const char* s, int scale) {
    int n = 0;
    for (; *s; s++) n++;
    return n > 0 ? n * 6 * scale - scale : 0;
}

static void draw_text(SDL_Renderer* r, const char* text, int x, int y, int scale,
                      struct nk_color col) {
    sdl_setcolor(r, col);
    int cx = x;
    for (const char* p = text; *p; p++) {
        const Glyph* g = find_glyph(*p);
        if (g) {
            for (int row = 0; row < 7; row++) {
                u8 bits = g->rows[row];
                for (int c = 0; c < 5; c++) {
                    if (bits & (1 << (4 - c))) {
                        SDL_Rect px = {cx + c * scale, y + row * scale, scale, scale};
                        SDL_RenderFillRect(r, &px);
                    }
                }
            }
        }
        cx += 6 * scale;
    }
}

static void draw_text_centered(SDL_Renderer* r, const char* text, int x, int y, int w,
                               int scale, struct nk_color col) {
    int tw = text_w_px(text, scale);
    draw_text(r, text, x + (w - tw) / 2, y, scale, col);
}
// THE EMU!!!!!
static const char* MASCOT[] = {
    "............................",
    "............................",
    "...........AAAA.............",
    "..........AAAAAA............",
    ".........AAAAAAA............",
    "........AAADAAAA............",
    ".......AAADDDAAAA...........",
    "......AADDDDDDDAA...........",
    ".....AADDDDDDDDDA...........",
    "....AdDDDDDDDDDDDA..........",
    "....dDDDDDDDDDDDDA..........",
    "....dDDFFFFFFFDDDA..........",
    "....dDDFFEEEEFFDDA..........",
    "....dDDFEEHPEEFDA...........",
    "....dDDFEEPPPEFDA..BBB......",
    "....dDDFEEEEEEFDABBBBBBb....",
    "....dDDFFEEEFFFBBBBBBbb.....",
    "....dDDDFFFFFDDDBBBb........",
    ".....dDDDDDDDDDDDb..........",
    ".....ddDDDDDDDDDD...........",
    "......dDDDDDDDDD............",
    ".......ddDDDDDD.............",
    "........ddDDDD..............",
    ".........NNNN...............",
    "........NNNNNN..............",
    "........NNNNNN..............",
    "........NNNNNN..............",
    "........NNNNNN..............",
    ".......NNNNNNNN.............",
    "......NNNNNNNNNN............",
};
#define MASCOT_W 28
#define MASCOT_H ((int)(sizeof(MASCOT) / sizeof(MASCOT[0])))

static struct nk_color mascot_color(char ch, const Theme* th) {
    switch (ch) {
        case 'A': return (struct nk_color){0x6e, 0x52, 0x42, 0xff};
        case 'D': return (struct nk_color){0x3a, 0x2a, 0x22, 0xff};
        case 'd': return (struct nk_color){0x22, 0x16, 0x12, 0xff};
        case 'F': return (struct nk_color){0xcc, 0xb6, 0x9a, 0xff};
        case 'E': return (struct nk_color){0xf6, 0xea, 0xd0, 0xff};
        case 'P': return (struct nk_color){0x12, 0x08, 0x04, 0xff};
        case 'H': return (struct nk_color){0xff, 0xff, 0xff, 0xff};
        case 'B': return (struct nk_color){0xea, 0x86, 0x2a, 0xff};
        case 'b': return (struct nk_color){0xa6, 0x48, 0x12, 0xff};
        case 'N': return blend(th->bezel, (struct nk_color){0x46,0x32,0x26,0xff}, 200);
        default:  return (struct nk_color){0,0,0,0};
    }
}

static void draw_mascot(SDL_Renderer* r, int x, int y, int scale, const Theme* th) {
    for (int row = 0; row < MASCOT_H; row++) {
        const char* line = MASCOT[row];
        for (int col = 0; col < MASCOT_W && line[col]; col++) {
            char c = line[col];
            if (c == '.' || c == ' ') continue;
            struct nk_color cc = mascot_color(c, th);
            if (cc.a == 0) continue;
            sdl_setcolor(r, cc);
            SDL_Rect px = {x + col * scale, y + row * scale, scale, scale};
            SDL_RenderFillRect(r, &px);
        }
    }
}

// SPLASH SCREEN
void ui_draw_splash(SDL_Renderer* r, SDL_Rect dst) {
    const Theme* th = T();

    sdl_setcolor(r, th->screen);
    SDL_RenderFillRect(r, &dst);

    sdl_setcolor(r, blend(th->screen, th->screen_dim, 24));
    for (int y = dst.y; y < dst.y + dst.h; y += 3) {
        SDL_Rect ln = {dst.x, y, dst.w, 1};
        SDL_RenderFillRect(r, &ln);
    }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    struct nk_color vig = th->screen_dim;
    for (int i = 0; i < 4; i++) {
        SDL_SetRenderDrawColor(r, vig.r, vig.g, vig.b, (Uint8)(40 - i * 8));
        SDL_Rect frame_t = {dst.x, dst.y + i, dst.w, 1};
        SDL_Rect frame_b = {dst.x, dst.y + dst.h - 1 - i, dst.w, 1};
        SDL_Rect frame_l = {dst.x + i, dst.y, 1, dst.h};
        SDL_Rect frame_r = {dst.x + dst.w - 1 - i, dst.y, 1, dst.h};
        SDL_RenderFillRect(r, &frame_t);
        SDL_RenderFillRect(r, &frame_b);
        SDL_RenderFillRect(r, &frame_l);
        SDL_RenderFillRect(r, &frame_r);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    int s = dst.h / (MASCOT_H + 4);
    if (s < 1) s = 1;
    if (s > 8) s = 8;
    int mw = MASCOT_W * s;
    int mh = MASCOT_H * s;
    int mx = dst.x + (dst.w - mw) / 2;
    int my = dst.y + dst.h / 7;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < 4; i++) {
        SDL_SetRenderDrawColor(r, th->shell_hi.r, th->shell_hi.g, th->shell_hi.b,
                               (Uint8)(28 - i * 6));
        SDL_Rect halo = {mx - 8 - i*2, my - 6 - i*2, mw + 16 + i*4, mh + 12 + i*4};
        SDL_RenderFillRect(r, &halo);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    draw_mascot(r, mx, my, s, th);

    int logo_scale = s > 1 ? s : 2;
    int logo_h = 7 * logo_scale;
    int logo_y = my + mh + logo_scale * 4;
    {
        const char* word = "GBEMU";
        int char_w = 5 * logo_scale;
        int gap    = logo_scale * 2;
        int n = (int)strlen(word);
        int total = n * char_w + (n - 1) * gap;
        int cx = dst.x + (dst.w - total) / 2;
        char buf[2] = {0, 0};
        for (int i = 0; i < n; i++) {
            buf[0] = word[i];
            draw_text(r, buf, cx, logo_y, logo_scale, th->screen_text);
            cx += char_w + gap;
        }
    }

    int hint_scale = s > 1 ? s - 1 : 1;
    int line_h = 7 * hint_scale + 4;
    int hint_y = logo_y + logo_h + line_h;
    Uint32 t = SDL_GetTicks();
    int phase = (int)(t / 8) & 0xFF;
    int tri = phase < 128 ? phase * 2 : (255 - phase) * 2;
    int blend_amt = 80 + (tri * 90) / 255;
    struct nk_color pulse = blend(th->screen_text, th->screen, blend_amt);
    draw_text_centered(r, "INSERT CARTRIDGE",
                       dst.x, hint_y, dst.w, hint_scale, pulse);
    draw_text_centered(r, "FILE > OPEN ROM",
                       dst.x, hint_y + line_h + 2, dst.w, hint_scale,
                       th->screen_dim);

    int tag_scale = hint_scale > 1 ? hint_scale - 1 : 1;
    int tag_y = hint_y + 2 * (line_h + 2) + 4;
    draw_text_centered(r, "PRESS ESC TO QUIT",
                       dst.x, tag_y, dst.w, tag_scale,
                       blend(th->screen_dim, th->screen, 80));

}
// FILE BROWSER
static int compare_entries(const void* a, const void* b) {
    const BrowserEntry* ea = (const BrowserEntry*)a;
    const BrowserEntry* eb = (const BrowserEntry*)b;
    if (ea->is_dir != eb->is_dir) return (int)eb->is_dir - (int)ea->is_dir;
    return strcmp(ea->name, eb->name);
}

static bool has_rom_ext(const char* name) {
    const char* dot = strrchr(name, '.');
    if (!dot) return false;
    return strcasecmp(dot, ".gb")  == 0
        || strcasecmp(dot, ".gbc") == 0
        || strcasecmp(dot, ".rom") == 0
        || strcasecmp(dot, ".bin") == 0;
}

static void refresh_browser(void) {
    browser_count = 0;
    browser_selected = -1;

    DIR* d = opendir(browser_dir);
    if (!d) {
        snprintf(browser_entries[0].name, sizeof(browser_entries[0].name),
                 "<cannot open dir>");
        browser_entries[0].is_dir = false;
        browser_count = 1;
        return;
    }

    strcpy(browser_entries[0].name, "..");
    browser_entries[0].is_dir = true;
    browser_count = 1;

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL && browser_count < MAX_FILES) {
        if (entry->d_name[0] == '.') continue;

        char full_path[MAX_PATH_LEN + 384];
        snprintf(full_path, sizeof(full_path), "%s/%s", browser_dir, entry->d_name);
        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        bool is_dir = S_ISDIR(st.st_mode);
        if (!is_dir && !has_rom_ext(entry->d_name)) continue;

        snprintf(browser_entries[browser_count].name,
                 sizeof(browser_entries[browser_count].name),
                 "%s", entry->d_name);
        browser_entries[browser_count].is_dir = is_dir;
        browser_count++;
    }
    closedir(d);
    qsort(browser_entries, browser_count, sizeof(BrowserEntry), compare_entries);
}

static void browser_navigate_up(void) {
    char* last = strrchr(browser_dir, '/');
    if (last && last != browser_dir) {
        *last = '\0';
    } else {
        strcpy(browser_dir, ".");
    }
    browser_dirty = true;
}

static void browser_enter(const char* name) {
    if (strcmp(name, "..") == 0) { browser_navigate_up(); return; }
    char tmp[MAX_PATH_LEN + 384];
    snprintf(tmp, sizeof(tmp), "%s/%s", browser_dir, name);
    strncpy(browser_dir, tmp, MAX_PATH_LEN - 1);
    browser_dir[MAX_PATH_LEN - 1] = '\0';
    browser_dirty = true;
}

static void open_selected_rom(void) {
    if (browser_selected < 0 || browser_selected >= browser_count) return;
    const BrowserEntry* e = &browser_entries[browser_selected];
    if (e->is_dir) { browser_enter(e->name); return; }
    char full_path[MAX_PATH_LEN + 384];
    snprintf(full_path, sizeof(full_path), "%s/%s", browser_dir, e->name);
    if (gameboy_load_rom(full_path)) show_file = false;
}

// DROPDOWNS

static bool dd_item(const char* label) {
    return nk_button_label_styled(ctx, &ctx->style.contextual_button, label) != 0;
}

static void dd_sep(const char* label) {
    nk_label_colored(ctx, label, NK_TEXT_LEFT, T()->text_alt);
}

static void draw_dropdown_items(int menu_idx) {
    nk_layout_row_dynamic(ctx, 20, 1);
    switch (menu_idx) {
        case 0:
            if (dd_item("Open ROM...")) { panel_open("Open ROM", &show_file, &reset_file_bounds); browser_dirty = true; active_menu = -1; }
            if (dd_item("Reset"))       { gameboy_reset();   active_menu = -1; }
            if (dd_item("Quit"))        { get_gb()->die = true; active_menu = -1; }
            break;
        case 1:
            if (dd_item(get_gb()->paused ? "Resume" : "Pause")) {
                get_gb()->paused = !get_gb()->paused;
                active_menu = -1;
            }
            dd_sep("Save State");
            for (int i = 1; i <= 4; i++) {
                char l[16]; snprintf(l, sizeof(l), "  Slot %d", i);
                if (dd_item(l)) { gameboy_save_state(i); active_menu = -1; }
            }
            dd_sep("Load State");
            for (int i = 1; i <= 4; i++) {
                char l[16]; snprintf(l, sizeof(l), "  Slot %d", i);
                if (dd_item(l)) { gameboy_load_state(i); active_menu = -1; }
            }
            break;
        case 2:
            if (dd_item("Toggle Fullscreen (F11)")) {
                Uint32 flags = SDL_GetWindowFlags(g_window);
                bool is_fs = (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
                SDL_SetWindowFullscreen(g_window, is_fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                active_menu = -1;
            }
            break;
        case 3:
            if (dd_item(show_audio ? "Hide Audio" : "Show Audio")) {
                if (show_audio) {
                    show_audio = false;
                    nk_window_close(ctx, "Audio");
                } else {
                    panel_open("Audio", &show_audio, &reset_audio_bounds);
                }
                active_menu = -1;
            }
            break;
        case 4:
            if (dd_item(show_cpu ? "Hide CPU" : "Show CPU")) {
                if (show_cpu) { show_cpu = false; nk_window_close(ctx, "CPU"); }
                else panel_open("CPU", &show_cpu, &reset_cpu_bounds);
                active_menu = -1;
            }
            if (dd_item(show_mem ? "Hide Memory" : "Show Memory")) {
                if (show_mem) { show_mem = false; nk_window_close(ctx, "Memory"); }
                else panel_open("Memory", &show_mem, &reset_mem_bounds);
                active_menu = -1;
            }
            break;
        case 5:
            if (dd_item(show_theme ? "Hide Theme Picker" : "Show Theme Picker")) {
                if (show_theme) { show_theme = false; nk_window_close(ctx, "Theme"); }
                else panel_open("Theme", &show_theme, &reset_theme_bounds);
                active_menu = -1;
            }
            break;
        case 6:
            if (dd_item("About")) { panel_open("About", &show_about, &reset_about_bounds); active_menu = -1; }
            break;
        default: break;
    }
}

static float text_w(const char* s) {
    if (!ctx || !ctx->style.font || !s) return 0.0f;
    const struct nk_user_font* f = ctx->style.font;
    return f->width(f->userdata, f->height, s, (int)strlen(s));
}

static inline float fmaxf2(float a, float b) { return a > b ? a : b; }

typedef struct {
    float max_w;
    int   rows;
    float total_h;
} Measure;

#define DD_ITEM_H 20
#define ITEM_PAD_W 16

static void m_item(Measure* m, const char* s) {
    m->max_w = fmaxf2(m->max_w, text_w(s) + ITEM_PAD_W);
    m->rows++;
    m->total_h += DD_ITEM_H;
}
static void m_sep(Measure* m, const char* s) {
    m->max_w = fmaxf2(m->max_w, text_w(s));
    m->rows++;
    m->total_h += DD_ITEM_H;
}
static struct nk_vec2 measure_finalize(const Measure* m) {
    float w = m->max_w + PANEL_PAD * 2 + BORDER_W * 2 + 4;
    float h = m->total_h;
    if (m->rows > 1) h += (m->rows - 1) * PANEL_SPACING;
    h += PANEL_PAD * 2 + BORDER_W * 2;
    return nk_vec2(w, h);
}

static struct nk_vec2 dropdown_size(int menu_idx) {
    Measure m = {0};
    char buf[64];

    switch (menu_idx) {
        case 0:
            m_item(&m, "Open ROM...");
            m_item(&m, "Reset");
            m_item(&m, "Quit");
            break;
        case 1:
            m_item(&m, get_gb()->paused ? "Resume" : "Pause");
            m_sep(&m,  "Save State");
            for (int i = 1; i <= 4; i++) {
                snprintf(buf, sizeof(buf), "  Slot %d", i);
                m_item(&m, buf);
            }
            m_sep(&m,  "Load State");
            for (int i = 1; i <= 4; i++) {
                snprintf(buf, sizeof(buf), "  Slot %d", i);
                m_item(&m, buf);
            }
            break;
        case 2:
            m_item(&m, "Toggle Fullscreen (F11)");
            break;
        case 3:
            m_item(&m, show_audio ? "Hide Audio" : "Show Audio");
            break;
        case 4:
            m_item(&m, show_cpu ? "Hide CPU" : "Show CPU");
            m_item(&m, show_mem ? "Hide Memory" : "Show Memory");
            break;
        case 5:
            m_item(&m, show_theme ? "Hide Theme Picker" : "Show Theme Picker");
            break;
        case 6:
            m_item(&m, "About");
            break;
        default: break;
    }
    return measure_finalize(&m);
}

static void draw_dropdown(int menu_idx) {
    int win_w, win_h;
    win_size(&win_w, &win_h);

    struct nk_rect hb = menu_header_bounds[menu_idx];
    struct nk_vec2 sz = dropdown_size(menu_idx);

    float x = hb.x;
    if (x + sz.x > (float)win_w) x = (float)win_w - sz.x;
    if (x < 0.0f) x = 0.0f;
    float y = hb.y + hb.h;
    if (y + sz.y > (float)win_h) y = hb.y - sz.y;
    if (y < 0.0f) y = 0.0f;

    dropdown_bounds = nk_rect(x, y, sz.x, sz.y);

    const char* name = DROPDOWN_NAMES[menu_idx];

    // NUKLEAR KEEPS OLD BOUNDS FORCE RESIZE EVERY FRAME
    nk_window_show(ctx, name, NK_SHOWN);
    nk_window_set_bounds(ctx, name, dropdown_bounds);

    if (nk_begin(ctx, name, dropdown_bounds,
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER | NK_WINDOW_DYNAMIC)) {
        draw_dropdown_items(menu_idx);
    }
    nk_end(ctx);
}

static void draw_main_menu(int win_w) {
    nk_style_push_vec2(ctx, &ctx->style.window.padding,
                       nk_vec2(MENUBAR_PAD_X, MENUBAR_PAD_Y));
    nk_style_push_vec2(ctx, &ctx->style.window.spacing,
                       nk_vec2(MENUBAR_SPACING, 0));

    if (!nk_begin(ctx, "_menubar",
                  nk_rect(0, 0, (float)win_w, MENU_H),
                  NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
        nk_end(ctx);
        nk_style_pop_vec2(ctx);
        nk_style_pop_vec2(ctx);
        return;
    }
    nk_window_set_bounds(ctx, "_menubar",
                         nk_rect(0, 0, (float)win_w, MENU_H));

    nk_layout_row_begin(ctx, NK_STATIC, MENU_H, MENU_COUNT + 2);

    for (int i = 0; i < MENU_COUNT; i++) {
        nk_layout_row_push(ctx, MENUS[i].width);
        menu_header_bounds[i] = nk_widget_bounds(ctx);

        bool is_active = (active_menu == i);
        struct nk_style_button btn = ctx->style.menu_button;
        if (is_active) {
            btn.normal = btn.active;
            btn.hover  = btn.active;
            btn.text_normal = btn.text_active;
            btn.text_hover  = btn.text_active;
        }
        if (nk_button_label_styled(ctx, &btn, MENUS[i].label)) {
            active_menu = is_active ? -1 : i;
        }

        if (active_menu != -1 && active_menu != i &&
            nk_input_is_mouse_hovering_rect(&ctx->input, menu_header_bounds[i])) {
            active_menu = i;
        }
    }

    float used = menus_total_w() + (float)WIN_BTN_W
               + 2.0f * (float)MENUBAR_PAD_X
               + (float)(MENU_COUNT + 1) * (float)MENUBAR_SPACING;
    float spacer = (float)win_w - used;
    if (spacer < 8.0f) spacer = 8.0f;
    nk_layout_row_push(ctx, spacer);
    {
        const char* rom = c.filename[0] ? c.filename : "(no rom)";
        const char* base = strrchr(rom, '/');
        base = base ? base + 1 : rom;
        char status[256];
        snprintf(status, sizeof(status), "GBEmu  [%s]  %.140s%s",
                 T()->name, base,
                 get_gb()->paused ? "  *PAUSED*" : "");
        nk_label_colored(ctx, status, NK_TEXT_RIGHT, T()->text);
    }

    nk_layout_row_push(ctx, WIN_BTN_W);
    {
        struct nk_style_button cb = ctx->style.button;
        cb.normal = nk_style_item_color(T()->shell);
        cb.hover  = nk_style_item_color((struct nk_color){0xc0, 0x30, 0x30, 0xff});
        cb.active = nk_style_item_color((struct nk_color){0x80, 0x20, 0x20, 0xff});
        cb.text_normal = T()->text;
        cb.text_hover  = (struct nk_color){0xff,0xff,0xff,0xff};
        cb.text_active = (struct nk_color){0xff,0xff,0xff,0xff};
        cb.border = 0;
        if (nk_button_label_styled(ctx, &cb, "X")) get_gb()->die = true;
    }

    nk_layout_row_end(ctx);
    nk_end(ctx);

    nk_style_pop_vec2(ctx);
    nk_style_pop_vec2(ctx);

    if (active_menu != -1) draw_dropdown(active_menu);

    if (active_menu != -1 &&
        nk_input_is_mouse_pressed(&ctx->input, NK_BUTTON_LEFT)) {
        bool inside = false;
        for (int i = 0; i < MENU_COUNT; i++) {
            if (nk_input_is_mouse_hovering_rect(&ctx->input, menu_header_bounds[i])) {
                inside = true; break;
            }
        }
        if (!inside &&
            nk_input_is_mouse_hovering_rect(&ctx->input, dropdown_bounds)) {
            inside = true;
        }
        if (!inside) active_menu = -1;
    }
}

// DYNAMIC FLAG MEANS RECT HEIGHT IS ONLY MAX
#define PANEL_FLAGS (NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE \
                   | NK_WINDOW_CLOSABLE | NK_WINDOW_SCALABLE \
                   | NK_WINDOW_DYNAMIC)

static void maybe_reset_panel(const char* name, struct nk_rect rect,
                              bool* reset_flag) {
    if (!*reset_flag) return;
    nk_window_set_bounds(ctx, name, rect);
    *reset_flag = false;
}

static void lblf(int rh, const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    nk_layout_row_dynamic(ctx, rh, 1);
    nk_label(ctx, buf, NK_TEXT_LEFT);
}

static void draw_audio_window(void) {
    struct nk_rect r = panel_rect(80, MENU_H + 20, 300, 260);
    maybe_reset_panel("Audio", r, &reset_audio_bounds);

    if (nk_begin(ctx, "Audio", r, PANEL_FLAGS)) {
        nk_layout_row_dynamic(ctx, ROW_H_LBL, 1);
        nk_label(ctx, "Master Volume", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, ROW_H_CTL, 1);
        float vol = apu_get_master_volume();
        if (nk_slider_float(ctx, 0.0f, &vol, 1.0f, 0.01f)) {
            apu_set_master_volume(vol);
        }
        char vbuf[16];
        snprintf(vbuf, sizeof(vbuf), "%d%%", (int)(vol * 100.0f + 0.5f));
        nk_layout_row_dynamic(ctx, ROW_H_LBL, 1);
        nk_label_colored(ctx, vbuf, NK_TEXT_RIGHT, T()->text_alt);

        nk_layout_row_dynamic(ctx, 4, 1);
        nk_spacing(ctx, 1);
        nk_layout_row_dynamic(ctx, ROW_H_LBL, 1);
        nk_label(ctx, "Channels", NK_TEXT_LEFT);

        const char* names[4] = {
            "1: Pulse + Sweep",
            "2: Pulse",
            "3: Wave",
            "4: Noise",
        };
        nk_layout_row_dynamic(ctx, ROW_H_CTL, 1);
        for (int i = 0; i < 4; i++) {
            int on = apu_get_channel_enabled(i) ? 1 : 0;
            nk_checkbox_label(ctx, names[i], &on);
            apu_set_channel_enabled(i, on != 0);
        }
    } else {
        show_audio = false;
    }
    nk_end(ctx);
}

static void draw_cpu_window(void) {
    struct nk_rect r = panel_rect(380, MENU_H + 20, 280, 260);
    maybe_reset_panel("CPU", r, &reset_cpu_bounds);

    if (nk_begin(ctx, "CPU", r, PANEL_FLAGS)) {
        registers* reg = get_registers();
        char buf[64];

        nk_layout_row_dynamic(ctx, ROW_H_LBL, 2);
        snprintf(buf, sizeof(buf), "A %02X", reg->a); nk_label(ctx, buf, NK_TEXT_LEFT);
        snprintf(buf, sizeof(buf), "F %02X", reg->f); nk_label(ctx, buf, NK_TEXT_LEFT);
        snprintf(buf, sizeof(buf), "B %02X", reg->b); nk_label(ctx, buf, NK_TEXT_LEFT);
        snprintf(buf, sizeof(buf), "C %02X", reg->c); nk_label(ctx, buf, NK_TEXT_LEFT);
        snprintf(buf, sizeof(buf), "D %02X", reg->d); nk_label(ctx, buf, NK_TEXT_LEFT);
        snprintf(buf, sizeof(buf), "E %02X", reg->e); nk_label(ctx, buf, NK_TEXT_LEFT);
        snprintf(buf, sizeof(buf), "H %02X", reg->h); nk_label(ctx, buf, NK_TEXT_LEFT);
        snprintf(buf, sizeof(buf), "L %02X", reg->l); nk_label(ctx, buf, NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 4, 1);
        nk_spacing(ctx, 1);

        lblf(ROW_H_LBL, "PC %04X   SP %04X", reg->pc, reg->sp);
        u8 f = reg->f;
        lblf(ROW_H_LBL, "Z %d  N %d  H %d  C %d",
             (f >> 7) & 1, (f >> 6) & 1, (f >> 5) & 1, (f >> 4) & 1);
        lblf(ROW_H_LBL, "IME %d   HALT %d",
             get_ime() ? 1 : 0, is_cpu_halted() ? 1 : 0);
    } else {
        show_cpu = false;
    }
    nk_end(ctx);
}

static void draw_mem_window(void) {
    struct nk_rect r = panel_rect(60, MENU_H + 300, 540, 300);
    maybe_reset_panel("Memory", r, &reset_mem_bounds);

    if (nk_begin(ctx, "Memory", r, PANEL_FLAGS)) {

        nk_layout_row_begin(ctx, NK_STATIC, ROW_H_CTL, 2);
        nk_layout_row_push(ctx, 64);
        nk_label(ctx, "Address", NK_TEXT_LEFT);
        nk_layout_row_push(ctx, 88);
        nk_edit_string(ctx, NK_EDIT_FIELD,
                       mem_addr_buf, &mem_addr_buf_len, 7, nk_filter_hex);
        nk_layout_row_end(ctx);

        u16 addr;
        {
            char tmp[8] = {0};
            int n = mem_addr_buf_len; if (n > 6) n = 6;
            if (n < 0) n = 0;
            memcpy(tmp, mem_addr_buf, n);
            addr = (u16)(strtoul(tmp, NULL, 16) & 0xFFFF);
        }
        addr &= ~0xF;

        nk_layout_row_dynamic(ctx, ROW_H_HEX, 1);
        char line[140];
        for (int row = 0; row < 12; row++) {
            u16 ra = addr + row * 16;
            int p = snprintf(line, sizeof(line), "%04X  ", ra);
            for (int col = 0; col < 16; col++) {
                p += snprintf(line + p, sizeof(line) - p,
                              "%02X ", read_from_bus(ra + col));
                if (col == 7)
                    p += snprintf(line + p, sizeof(line) - p, " ");
            }
            p += snprintf(line + p, sizeof(line) - p, " ");
            for (int col = 0; col < 16; col++) {
                u8 b = read_from_bus(ra + col);
                p += snprintf(line + p, sizeof(line) - p, "%c",
                              (b >= 0x20 && b < 0x7F) ? (char)b : '.');
            }
            nk_label(ctx, line, NK_TEXT_LEFT);
        }
    } else {
        show_mem = false;
    }
    nk_end(ctx);
}

static void draw_about_window(void) {
    struct nk_rect r = panel_rect(180, MENU_H + 80, 360, 240);
    maybe_reset_panel("About", r, &reset_about_bounds);

    if (nk_begin(ctx, "About", r,
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE |
                 NK_WINDOW_CLOSABLE | NK_WINDOW_DYNAMIC)) {

        nk_layout_row_dynamic(ctx, ROW_H_LBL + 8, 1);
        nk_label_colored(ctx, "GBEmu", NK_TEXT_CENTERED, T()->text);

        nk_layout_row_dynamic(ctx, ROW_H_LBL, 1);
        nk_label_colored(ctx, "Pure C like god intended",
                         NK_TEXT_CENTERED, T()->text_alt);

        nk_layout_row_dynamic(ctx, 6, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, ROW_H_LBL, 1);
        nk_label(ctx, "Controls", NK_TEXT_CENTERED);
        nk_layout_row_dynamic(ctx, ROW_H_LBL, 1);
        nk_label_colored(ctx, "ARROW KEYS / WASD = D-Pad", NK_TEXT_CENTERED, T()->text_alt);
        nk_label_colored(ctx, "Z / X = A / B", NK_TEXT_CENTERED, T()->text_alt);
        nk_label_colored(ctx, "ENTER / RSHIFT = Start / Select", NK_TEXT_CENTERED, T()->text_alt);
        nk_label_colored(ctx, "F11 = Fullscreen   [ / ] = cycle theme",
                         NK_TEXT_CENTERED, T()->text_alt);

        nk_layout_row_dynamic(ctx, 6, 1);
        nk_spacing(ctx, 1);

        char buf[64];
        snprintf(buf, sizeof(buf), "Theme: %s  (%d / %d)",
                 T()->name, active_theme + 1, THEME_COUNT);
        nk_layout_row_dynamic(ctx, ROW_H_LBL, 1);
        nk_label_colored(ctx, buf, NK_TEXT_CENTERED, T()->text_alt);
    } else {
        show_about = false;
    }
    nk_end(ctx);
}

static void draw_theme_window(void) {
    const int rows = (THEME_COUNT + 1) / 2;
    const int row_h = ROW_H_CTL + 2;
    int height = PANEL_PAD * 2 + rows * row_h + (rows - 1) * PANEL_SPACING + 32;
    struct nk_rect r = panel_rect(120, MENU_H + 40, 280, (float)height);
    maybe_reset_panel("Theme", r, &reset_theme_bounds);

    if (nk_begin(ctx, "Theme", r, PANEL_FLAGS)) {
        nk_layout_row_dynamic(ctx, ROW_H_LBL, 1);
        char hdr[64];
        snprintf(hdr, sizeof(hdr), "Active: %s", T()->name);
        nk_label_colored(ctx, hdr, NK_TEXT_LEFT, T()->text_alt);

        nk_layout_row_dynamic(ctx, ROW_H_CTL, 2);
        for (int i = 0; i < THEME_COUNT; i++) {
            const Theme* th = &THEMES[i];
            struct nk_style_button btn = ctx->style.button;
            bool active = (i == active_theme);
            btn.normal       = nk_style_item_color(active ? th->accent : th->shell);
            btn.hover        = nk_style_item_color(th->accent);
            btn.active       = nk_style_item_color(th->accent);
            btn.border_color = th->shell_lo;
            btn.text_normal  = active ? (struct nk_color){0xff,0xff,0xff,0xff} : th->text;
            btn.text_hover   = (struct nk_color){0xff,0xff,0xff,0xff};
            btn.text_active  = (struct nk_color){0xff,0xff,0xff,0xff};
            btn.text_alignment = NK_TEXT_CENTERED;
            btn.border       = active ? 2 : 1;
            btn.rounding     = 0;
            btn.padding      = nk_vec2(4, 2);

            if (nk_button_label_styled(ctx, &btn, th->name)) {
                active_theme = i;
            }
        }
    } else {
        show_theme = false;
    }
    nk_end(ctx);
}

static void draw_file_browser(void) {
    if (browser_dirty) {
        refresh_browser();
        browser_dirty = false;
    }

    struct nk_rect r = panel_rect(60, MENU_H + 20, 460, 400);
    maybe_reset_panel("Open ROM", r, &reset_file_bounds);

    if (nk_begin(ctx, "Open ROM", r, PANEL_FLAGS)) {

        nk_layout_row_dynamic(ctx, ROW_H_LBL, 1);
        char dir_label[256];
        char dir_short[200];
        size_t n = strlen(browser_dir);
        if (n > sizeof(dir_short) - 1) {
            snprintf(dir_short, sizeof(dir_short), "...%s",
                     browser_dir + n - (sizeof(dir_short) - 4));
        } else {
            snprintf(dir_short, sizeof(dir_short), "%s", browser_dir);
        }
        snprintf(dir_label, sizeof(dir_label), "DIR  %s", dir_short);
        nk_label_colored(ctx, dir_label, NK_TEXT_LEFT, T()->text_alt);

        nk_layout_row_dynamic(ctx, 280, 1);
        if (nk_group_begin(ctx, "entries", NK_WINDOW_BORDER)) {
            nk_layout_row_dynamic(ctx, 18, 1);
            for (int i = 0; i < browser_count; i++) {
                char label[320];
                snprintf(label, sizeof(label), "%s  %.280s",
                         browser_entries[i].is_dir ? "[D]" : "   ",
                         browser_entries[i].name);
                int sel = (i == browser_selected);
                if (nk_selectable_label(ctx, label, NK_TEXT_LEFT, &sel)) {
                    browser_selected = sel ? i : -1;
                }
            }
            nk_group_end(ctx);
        }

        nk_layout_row_dynamic(ctx, ROW_H_CTL + 2, 2);
        if (nk_button_label(ctx, "Cancel")) show_file = false;
        const char* action = "Load";
        if (browser_selected >= 0 && browser_entries[browser_selected].is_dir) {
            action = "Enter";
        }
        if (nk_button_label(ctx, action)) open_selected_rom();
    } else {
        show_file = false;
    }
    nk_end(ctx);
}

void ui_init(SDL_Window* window, SDL_Renderer* renderer) {
    g_window = window;
    g_renderer = renderer;
    ctx = nk_sdl_init(window, renderer);

    struct nk_font_atlas* atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();

    ui_initialized = true;
}

void ui_shutdown(void) {
    if (!ui_initialized) return;
    if (g_grain_tex) {
        SDL_DestroyTexture(g_grain_tex);
        g_grain_tex = NULL;
        g_grain_theme = -1;
    }
    nk_sdl_shutdown();
    ui_initialized = false;
    ctx = NULL;
}

void ui_input_begin(void) {
    if (!ui_initialized) return;
    nk_input_begin(ctx);
}

void ui_input_end(void) {
    if (!ui_initialized) return;
    nk_input_end(ctx);
}

bool ui_handle_event(const SDL_Event* event) {
    if (!ui_initialized) return false;
    nk_sdl_handle_event((SDL_Event*)event);

    if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT) return false;

    if (event->type == SDL_KEYDOWN && !nk_item_is_any_active(ctx)) {
        SDL_Keycode k = event->key.keysym.sym;
        if (k == SDLK_LEFTBRACKET)  { cycle_theme(-1); return true; }
        if (k == SDLK_RIGHTBRACKET) { cycle_theme(+1); return true; }
    }

    if (event->type == SDL_MOUSEBUTTONDOWN ||
        event->type == SDL_MOUSEBUTTONUP   ||
        event->type == SDL_MOUSEMOTION     ||
        event->type == SDL_MOUSEWHEEL) {
        return nk_window_is_any_hovered(ctx);
    }
    if (event->type == SDL_KEYDOWN ||
        event->type == SDL_KEYUP   ||
        event->type == SDL_TEXTINPUT) {
        return nk_item_is_any_active(ctx);
    }
    return false;
}

bool ui_wants_keyboard(void) {
    return ui_initialized && nk_item_is_any_active(ctx);
}
bool ui_wants_mouse(void) {
    return ui_initialized && nk_window_is_any_hovered(ctx);
}

int ui_menu_height(void) {
    return window_is_fullscreen() ? 0 : MENU_H;
}

SDL_HitTestResult ui_hit_test(int x, int y, int win_w, int win_h) {
    if (window_is_fullscreen()) return SDL_HITTEST_NORMAL;

    if (x >= win_w - DRAG_CORNER_SZ && y >= win_h - DRAG_CORNER_SZ) {
        return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    }

    bool t = y < RESIZE_BORDER, b = y > win_h - RESIZE_BORDER;
    bool l = x < RESIZE_BORDER, r = x > win_w - RESIZE_BORDER;
    if (t && l) return SDL_HITTEST_RESIZE_TOPLEFT;
    if (t && r) return SDL_HITTEST_RESIZE_TOPRIGHT;
    if (b && l) return SDL_HITTEST_RESIZE_BOTTOMLEFT;
    if (b && r) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    if (t) return SDL_HITTEST_RESIZE_TOP;
    if (b) return SDL_HITTEST_RESIZE_BOTTOM;
    if (l) return SDL_HITTEST_RESIZE_LEFT;
    if (r) return SDL_HITTEST_RESIZE_RIGHT;

    const int menu_zone_end =
        MENUBAR_PAD_X + (int)menus_total_w()
        + MENU_COUNT * MENUBAR_SPACING;
    if (y < MENU_H && x >= menu_zone_end && x < win_w - WIN_BTN_W - MENUBAR_PAD_X) {
        return SDL_HITTEST_DRAGGABLE;
    }
    return SDL_HITTEST_NORMAL;
}

void ui_new_frame(void) {
    if (!ui_initialized) return;
    apply_theme();

    int win_w, win_h;
    win_size(&win_w, &win_h);

    bool show_menu = menubar_should_show();
    if (!show_menu) {
        active_menu = -1;
    } else {
        draw_main_menu(win_w);
    }
    menubar_visible_last_frame = show_menu;

    if (show_audio) draw_audio_window();
    if (show_theme) draw_theme_window();
    if (show_cpu)   draw_cpu_window();
    if (show_mem)   draw_mem_window();
    if (show_file)  draw_file_browser();
    if (show_about) draw_about_window();
}

void ui_render(void) {
    if (!ui_initialized) return;
    nk_sdl_render(NK_ANTI_ALIASING_ON);
}
