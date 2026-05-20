#ifndef UI_H
#define UI_H

#include "config.h"
#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_init(SDL_Window* window, SDL_Renderer* renderer);
void ui_shutdown(void);

void ui_input_begin(void);
void ui_input_end(void);
bool ui_handle_event(const SDL_Event* event);

void ui_new_frame(void);
void ui_render(void);

bool ui_wants_keyboard(void);
bool ui_wants_mouse(void);

int      ui_menu_height(void);
SDL_Rect ui_screen_rect(int win_w, int win_h);
void     ui_draw_chrome_under(SDL_Renderer* r, int win_w, int win_h);
void     ui_draw_chrome_over(SDL_Renderer* r, int win_w, int win_h);
void     ui_draw_shadows(SDL_Renderer* r);
void     ui_draw_splash(SDL_Renderer* r, SDL_Rect screen);
void     ui_default_window_size(int* w, int* h);
void     ui_min_window_size(int* w, int* h);

SDL_HitTestResult ui_hit_test(int x, int y, int win_w, int win_h);

#ifdef __cplusplus
}
#endif

#endif
