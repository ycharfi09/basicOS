#include "gui.h"
#include "../../drivers/include/framebuffer.h"
#include "../../drivers/include/keyboard.h"
#include "../../drivers/include/mouse.h"
#include "../../kernel/include/memory.h"
#include <stdint.h>
#include <stdbool.h>

/* Forward declarations for apps */
extern void app_terminal_create(void);
extern void app_editor_create(void);
extern void app_settings_create(void);
extern void app_files_create(void);
extern void app_game_create(void);

/* Maximum number of windows */
#define MAX_WINDOWS 16

/* Window list */
static window_t *windows[MAX_WINDOWS];
static int window_count = 0;
static window_t *focused_window = NULL;

/* Top bar state */
static bool launcher_open = false;

/* String functions */
static int strlen(const char *str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

static void strncpy(char *dest, const char *src, int n) {
    int i;
    for (i = 0; i < n - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/* Initialize GUI */
void gui_init(void) {
    window_count = 0;
    focused_window = NULL;
    launcher_open = false;
}

/* Create a new window */
window_t *gui_create_window(const char *title, int x, int y, int width, int height) {
    if (window_count >= MAX_WINDOWS) {
        return NULL;
    }

    window_t *win = (window_t *)kmalloc(sizeof(window_t));
    if (!win) {
        return NULL;
    }

    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    strncpy(win->title, title, 63);
    win->bg_color = RGB(240, 240, 240);
    win->mode = WINDOW_MODE_DRAGGABLE;
    win->visible = true;
    win->focused = false;
    win->render = NULL;
    win->update = NULL;
    win->on_key = NULL;
    win->on_click = NULL;
    win->data = NULL;

    windows[window_count++] = win;
    gui_focus_window(win);

    return win;
}

/* Close a window */
void gui_close_window(window_t *win) {
    for (int i = 0; i < window_count; i++) {
        if (windows[i] == win) {
            /* Shift remaining windows */
            for (int j = i; j < window_count - 1; j++) {
                windows[j] = windows[j + 1];
            }
            window_count--;
            
            if (focused_window == win) {
                focused_window = window_count > 0 ? windows[window_count - 1] : NULL;
            }
            
            kfree(win);
            break;
        }
    }
}

/* Focus a window */
void gui_focus_window(window_t *win) {
    if (focused_window) {
        focused_window->focused = false;
    }
    focused_window = win;
    if (win) {
        win->focused = true;
    }
}

/* Set window mode */
void gui_set_window_mode(window_t *win, window_mode_t mode) {
    win->mode = mode;
}

/* Draw top bar */
static void gui_draw_topbar(void) {
    int bar_height = 32;
    
    /* Draw bar background (semi-transparent gray simulation) */
    fb_draw_rect(0, 0, fb_get_width(), bar_height, RGB(60, 60, 60));

    /* Draw launcher button */
    gui_draw_button(8, 4, 80, 24, "Apps", launcher_open);

    /* Draw clock in center */
    char time_str[16] = "12:00";  /* Placeholder */
    fb_draw_string(fb_get_width() / 2 - 20, 12, time_str, COLOR_WHITE);

    /* Draw status icons (placeholders) */
    fb_draw_string(fb_get_width() - 60, 12, "[*]", COLOR_WHITE);
}

/* Draw launcher menu */
static void gui_draw_launcher(void) {
    if (!launcher_open) return;

    int menu_x = 8;
    int menu_y = 36;
    int menu_width = 200;
    int menu_height = 250;

    /* Draw menu background */
    fb_draw_rect(menu_x, menu_y, menu_width, menu_height, RGB(50, 50, 50));
    fb_draw_rect(menu_x + 1, menu_y + 1, menu_width - 2, menu_height - 2, RGB(70, 70, 70));

    /* Draw menu items */
    const char *items[] = {
        "Terminal",
        "Text Editor",
        "Settings",
        "File Manager",
        "Demo Game"
    };
    
    for (int i = 0; i < 5; i++) {
        fb_draw_string(menu_x + 10, menu_y + 10 + i * 30, items[i], COLOR_WHITE);
    }
}

/* Draw window frame */
void gui_draw_window_frame(window_t *win) {
    if (!win->visible) return;

    /* Title bar */
    int title_height = 30;
    uint32_t title_color = win->focused ? RGB(80, 120, 200) : RGB(120, 120, 120);
    fb_draw_rect(win->x, win->y, win->width, title_height, title_color);

    /* Title text */
    fb_draw_string(win->x + 10, win->y + 10, win->title, COLOR_WHITE);

    /* Close button */
    fb_draw_rect(win->x + win->width - 25, win->y + 5, 20, 20, RGB(200, 80, 80));
    fb_draw_string(win->x + win->width - 21, win->y + 8, "X", COLOR_WHITE);

    /* Window background */
    fb_draw_rect(win->x, win->y + title_height, win->width, win->height - title_height, win->bg_color);

    /* Window border */
    /* Top */
    fb_draw_rect(win->x, win->y, win->width, 2, COLOR_BLACK);
    /* Bottom */
    fb_draw_rect(win->x, win->y + win->height - 2, win->width, 2, COLOR_BLACK);
    /* Left */
    fb_draw_rect(win->x, win->y, 2, win->height, COLOR_BLACK);
    /* Right */
    fb_draw_rect(win->x + win->width - 2, win->y, 2, win->height, COLOR_BLACK);
}

/* Draw button */
void gui_draw_button(int x, int y, int width, int height, const char *text, bool pressed) {
    uint32_t color = pressed ? RGB(100, 100, 100) : RGB(120, 120, 120);
    fb_draw_rect(x, y, width, height, color);
    
    /* Border */
    uint32_t border = pressed ? RGB(60, 60, 60) : RGB(150, 150, 150);
    fb_draw_rect(x, y, width, 2, border);
    fb_draw_rect(x, y + height - 2, width, 2, border);
    fb_draw_rect(x, y, 2, height, border);
    fb_draw_rect(x + width - 2, y, 2, height, border);

    /* Text centered */
    int text_x = x + (width - strlen(text) * 8) / 2;
    int text_y = y + (height - 8) / 2;
    fb_draw_string(text_x, text_y, text, COLOR_WHITE);
}

/* Update GUI state */
void gui_update(void) {
    /* Handle keyboard input */
    if (keyboard_has_key()) {
        char key = keyboard_get_key();
        if (focused_window && focused_window->on_key) {
            focused_window->on_key(focused_window, key);
        }
    }

    /* Handle mouse input */
    struct mouse_state mouse;
    mouse_get_state(&mouse);

    /* Check for clicks on launcher button */
    static bool last_left_button = false;
    if (mouse.left_button && !last_left_button) {
        /* Check launcher button click */
        if (mouse.x >= 8 && mouse.x <= 88 && mouse.y >= 4 && mouse.y <= 28) {
            launcher_open = !launcher_open;
        }

        /* Check launcher menu clicks */
        if (launcher_open) {
            int menu_x = 8;
            int menu_y = 36;
            int menu_width = 200;
            
            if (mouse.x >= menu_x && mouse.x <= menu_x + menu_width) {
                /* Check which item was clicked */
                int item_index = (mouse.y - menu_y - 10) / 30;
                if (item_index >= 0 && item_index < 5 && mouse.y >= menu_y) {
                    launcher_open = false;
                    
                    /* Launch app based on index */
                    switch (item_index) {
                        case 0: app_terminal_create(); break;
                        case 1: app_editor_create(); break;
                        case 2: app_settings_create(); break;
                        case 3: app_files_create(); break;
                        case 4: app_game_create(); break;
                    }
                }
            }
        }

        /* Check window clicks */
        if (focused_window) {
            /* Check close button */
            int close_x = focused_window->x + focused_window->width - 25;
            int close_y = focused_window->y + 5;
            if (mouse.x >= close_x && mouse.x <= close_x + 20 &&
                mouse.y >= close_y && mouse.y <= close_y + 20) {
                gui_close_window(focused_window);
            } else if (focused_window->on_click) {
                focused_window->on_click(focused_window, mouse.x, mouse.y);
            }
        }
    }
    last_left_button = mouse.left_button;

    /* Update windows */
    for (int i = 0; i < window_count; i++) {
        if (windows[i]->update) {
            windows[i]->update(windows[i]);
        }
    }
}

/* Render GUI */
void gui_render(void) {
    /* Clear background */
    fb_clear(RGB(20, 30, 50));

    /* Render windows (back to front) */
    for (int i = 0; i < window_count; i++) {
        if (!windows[i]->visible) continue;

        gui_draw_window_frame(windows[i]);

        if (windows[i]->render) {
            windows[i]->render(windows[i]);
        }
    }

    /* Draw top bar */
    gui_draw_topbar();

    /* Draw launcher menu */
    gui_draw_launcher();

    /* Draw mouse cursor */
    struct mouse_state mouse;
    mouse_get_state(&mouse);
    
    /* Simple arrow cursor */
    for (int i = 0; i < 10; i++) {
        fb_putpixel(mouse.x, mouse.y + i, COLOR_WHITE);
        fb_putpixel(mouse.x + i / 2, mouse.y + i, COLOR_WHITE);
    }

    /* Swap back buffer to screen */
    fb_swap();
}
