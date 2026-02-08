#include "apps.h"
#include "../gui/include/gui.h"
#include "../drivers/include/framebuffer.h"

/* Game render function */
static void game_render(window_t *win) {
    /* Draw game title */
    fb_draw_string(win->x + win->width / 2 - 80, win->y + 50, "Demo Game Window", COLOR_BLACK);
    
    /* Draw placeholder content */
    fb_draw_string(win->x + 20, win->y + 100, "Fireboy & Watergirl", COLOR_RED);
    fb_draw_string(win->x + 20, win->y + 120, "(Placeholder)", COLOR_GRAY);
    
    /* Draw some colored rectangles to simulate a game */
    fb_draw_rect(win->x + 50, win->y + 150, 40, 40, RGB(255, 100, 50));  /* Fireboy */
    fb_draw_rect(win->x + 120, win->y + 150, 40, 40, RGB(50, 150, 255)); /* Watergirl */
    
    /* Draw "level" */
    for (int i = 0; i < 10; i++) {
        fb_draw_rect(win->x + 30 + i * 30, win->y + 200, 20, 20, RGB(100, 80, 60));
    }
    
    fb_draw_string(win->x + 20, win->y + win->height - 60, "Use arrow keys to play", COLOR_GRAY);
}

/* Create demo game app */
void app_game_create(void) {
    window_t *win = gui_create_window("Demo Game", 300, 150, 500, 400);
    if (!win) return;

    win->render = game_render;
}
