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

/* Title bar height constant */
#define TITLE_BAR_HEIGHT 30

/* Top bar height constant */
#define TOP_BAR_HEIGHT 32

/* Window list */
static window_t *windows[MAX_WINDOWS];
static int window_count = 0;
static window_t *focused_window = NULL;

/* Z-order counter (increases with each focus) */
static int next_z_order = 0;

/* Top bar state */
static bool launcher_open = false;

/* Track which window is being dragged (NULL if none) */
static window_t *dragging_window = NULL;

/* Previous mouse state for edge detection */
static bool last_left_button = false;

/* String functions */
static int gui_strlen(const char *str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

static void gui_strncpy(char *dest, const char *src, int n) {
    int i;
    for (i = 0; i < n - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/* ---- Z-order sorting ---- */

/* Sort the windows array by z_order (low to high) for back-to-front rendering */
static void gui_sort_windows_by_z(void) {
    for (int i = 0; i < window_count - 1; i++) {
        for (int j = 0; j < window_count - i - 1; j++) {
            if (windows[j]->z_order > windows[j + 1]->z_order) {
                window_t *tmp = windows[j];
                windows[j] = windows[j + 1];
                windows[j + 1] = tmp;
            }
        }
    }
}

/* ---- Hit testing ---- */

/* Check if point (px, py) is inside a window's title bar */
static bool point_in_title_bar(window_t *win, int px, int py) {
    return (px >= win->x && px < win->x + win->width &&
            py >= win->y && py < win->y + TITLE_BAR_HEIGHT);
}

/* Check if point (px, py) is inside a window's close button */
static bool point_in_close_button(window_t *win, int px, int py) {
    int cx = win->x + win->width - 25;
    int cy = win->y + 5;
    return (px >= cx && px < cx + 20 && py >= cy && py < cy + 20);
}

/* Check if point (px, py) is inside a window's area */
static bool point_in_window(window_t *win, int px, int py) {
    return (px >= win->x && px < win->x + win->width &&
            py >= win->y && py < win->y + win->height);
}

/* Find topmost window at point (px, py), by z_order descending */
static window_t *window_at_point(int px, int py) {
    window_t *top = NULL;
    int top_z = -1;
    for (int i = 0; i < window_count; i++) {
        if (windows[i]->visible && point_in_window(windows[i], px, py)) {
            if (windows[i]->z_order > top_z) {
                top_z = windows[i]->z_order;
                top = windows[i];
            }
        }
    }
    return top;
}

/* ---- Screen bounds clamping ---- */

/* Prevent window from moving outside screen bounds */
static void clamp_window_position(window_t *win) {
    int screen_w = (int)fb_get_width();
    int screen_h = (int)fb_get_height();

    /* Keep at least 50px of the window visible */
    if (win->x < -win->width + 50) win->x = -win->width + 50;
    if (win->y < TOP_BAR_HEIGHT) win->y = TOP_BAR_HEIGHT;
    if (win->x > screen_w - 50) win->x = screen_w - 50;
    if (win->y > screen_h - 50) win->y = screen_h - 50;
}

/* Initialize GUI */
void gui_init(void) {
    window_count = 0;
    focused_window = NULL;
    launcher_open = false;
    dragging_window = NULL;
    next_z_order = 0;
    last_left_button = false;
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
    win->saved_x = x;
    win->saved_y = y;
    win->saved_width = width;
    win->saved_height = height;
    gui_strncpy(win->title, title, 63);
    win->bg_color = RGB(240, 240, 240);
    win->mode = WINDOW_MODE_DRAGGABLE;
    win->visible = true;
    win->focused = false;
    win->dragging = false;
    win->drag_offset_x = 0;
    win->drag_offset_y = 0;
    win->z_order = next_z_order++;
    win->quit_confirm_pending = false;
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
                focused_window = NULL;
                /* Focus the topmost remaining window */
                if (window_count > 0) {
                    int top_z = -1;
                    for (int k = 0; k < window_count; k++) {
                        if (windows[k]->z_order > top_z) {
                            top_z = windows[k]->z_order;
                            focused_window = windows[k];
                        }
                    }
                    if (focused_window) {
                        focused_window->focused = true;
                    }
                }
            }

            if (dragging_window == win) {
                dragging_window = NULL;
            }

            kfree(win);
            break;
        }
    }
}

/* Focus a window - brings it to front */
void gui_focus_window(window_t *win) {
    if (focused_window && focused_window != win) {
        focused_window->focused = false;
    }
    focused_window = win;
    if (win) {
        win->focused = true;
        win->z_order = next_z_order++;
        gui_sort_windows_by_z();
    }
}

/* Set window mode */
void gui_set_window_mode(window_t *win, window_mode_t mode) {
    if (win->mode == mode) return;

    window_mode_t old_mode = win->mode;

    /* Leaving focused mode: restore saved position/size */
    if (old_mode == WINDOW_MODE_FOCUSED) {
        win->x = win->saved_x;
        win->y = win->saved_y;
        win->width = win->saved_width;
        win->height = win->saved_height;
    }

    win->mode = mode;
    win->quit_confirm_pending = false;

    /* Entering focused mode: save position and go fullscreen */
    if (mode == WINDOW_MODE_FOCUSED) {
        win->saved_x = win->x;
        win->saved_y = win->y;
        win->saved_width = win->width;
        win->saved_height = win->height;
        win->x = 0;
        win->y = TOP_BAR_HEIGHT;
        win->width = (int)fb_get_width();
        win->height = (int)fb_get_height() - TOP_BAR_HEIGHT;
        gui_focus_window(win);
    }
}

/* Draw top bar */
static void gui_draw_topbar(void) {
    /* Draw bar background */
    fb_draw_rect(0, 0, fb_get_width(), TOP_BAR_HEIGHT, RGB(60, 60, 60));

    /* In focused mode, show app title in top bar as menu bar */
    if (focused_window && focused_window->mode == WINDOW_MODE_FOCUSED) {
        gui_draw_button(8, 4, 80, 24, "Apps", launcher_open);
        fb_draw_string(100, 12, focused_window->title, COLOR_WHITE);
        /* Show mode indicator */
        fb_draw_string(fb_get_width() - 160, 12, "[Focused]", COLOR_CYAN);
    } else {
        /* Normal top bar: launcher button */
        gui_draw_button(8, 4, 80, 24, "Apps", launcher_open);
    }

    /* Draw clock in center */
    fb_draw_string(fb_get_width() / 2 - 20, 12, "12:00", COLOR_WHITE);

    /* Draw status icons */
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

    /* In focused mode, no title bar decorations (top bar acts as menu bar) */
    if (win->mode == WINDOW_MODE_FOCUSED) {
        /* Window fills below top bar with no frame */
        fb_draw_rect(win->x, win->y, win->width, win->height, win->bg_color);
        return;
    }

    /* Title bar */
    uint32_t title_color = win->focused ? RGB(80, 120, 200) : RGB(120, 120, 120);
    fb_draw_rect(win->x, win->y, win->width, TITLE_BAR_HEIGHT, title_color);

    /* Title text */
    fb_draw_string(win->x + 10, win->y + 10, win->title, COLOR_WHITE);

    /* Close button */
    fb_draw_rect(win->x + win->width - 25, win->y + 5, 20, 20, RGB(200, 80, 80));
    fb_draw_string(win->x + win->width - 21, win->y + 8, "X", COLOR_WHITE);

    /* Mode toggle button (square icon next to close) for switching to focused */
    fb_draw_rect(win->x + win->width - 50, win->y + 5, 20, 20, RGB(80, 160, 80));
    fb_draw_string(win->x + win->width - 46, win->y + 8, "F", COLOR_WHITE);

    /* Window background */
    fb_draw_rect(win->x, win->y + TITLE_BAR_HEIGHT, win->width,
                 win->height - TITLE_BAR_HEIGHT, win->bg_color);

    /* Window border */
    fb_draw_rect(win->x, win->y, win->width, 2, COLOR_BLACK);
    fb_draw_rect(win->x, win->y + win->height - 2, win->width, 2, COLOR_BLACK);
    fb_draw_rect(win->x, win->y, 2, win->height, COLOR_BLACK);
    fb_draw_rect(win->x + win->width - 2, win->y, 2, win->height, COLOR_BLACK);

    /* Quit confirmation overlay for focused mode transition */
    if (win->quit_confirm_pending) {
        fb_draw_rect(win->x + 10, win->y + TITLE_BAR_HEIGHT + 5, win->width - 20, 20,
                     RGB(200, 60, 60));
        fb_draw_string(win->x + 15, win->y + TITLE_BAR_HEIGHT + 9,
                       "Click X again to close", COLOR_WHITE);
    }
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
    int text_x = x + (width - gui_strlen(text) * 8) / 2;
    int text_y = y + (height - 8) / 2;
    fb_draw_string(text_x, text_y, text, COLOR_WHITE);
}

/* ---- Input handling ---- */

/* Handle mouse button press (transition from not-pressed to pressed) */
static void handle_mouse_press(int mx, int my) {
    /* Check launcher button click */
    if (mx >= 8 && mx <= 88 && my >= 4 && my <= 28) {
        launcher_open = !launcher_open;
        return;
    }

    /* Check launcher menu clicks */
    if (launcher_open) {
        int menu_x = 8;
        int menu_y = 36;
        int menu_width = 200;

        if (mx >= menu_x && mx <= menu_x + menu_width && my >= menu_y) {
            int item_index = (my - menu_y - 10) / 30;
            if (item_index >= 0 && item_index < 5) {
                launcher_open = false;
                switch (item_index) {
                    case 0: app_terminal_create(); break;
                    case 1: app_editor_create(); break;
                    case 2: app_settings_create(); break;
                    case 3: app_files_create(); break;
                    case 4: app_game_create(); break;
                }
                return;
            }
        }

        /* Click outside launcher closes it */
        launcher_open = false;
        return;
    }

    /* In focused mode, the focused window captures all input */
    if (focused_window && focused_window->mode == WINDOW_MODE_FOCUSED) {
        if (focused_window->on_click) {
            focused_window->on_click(focused_window, mx, my);
        }
        return;
    }

    /* Find topmost window under cursor */
    window_t *clicked = window_at_point(mx, my);
    if (!clicked) {
        return;
    }

    /* Focus the clicked window (brings it to front) */
    if (clicked != focused_window) {
        gui_focus_window(clicked);
    }

    /* Check close button */
    if (point_in_close_button(clicked, mx, my)) {
        if (clicked->quit_confirm_pending) {
            /* Double-confirm: actually close */
            gui_close_window(clicked);
        } else {
            /* First click: set pending */
            clicked->quit_confirm_pending = true;
        }
        return;
    }

    /* Reset quit confirm if clicking elsewhere */
    clicked->quit_confirm_pending = false;

    /* Check fullscreen toggle button (next to close) */
    int fx = clicked->x + clicked->width - 50;
    int fy = clicked->y + 5;
    if (mx >= fx && mx < fx + 20 && my >= fy && my < fy + 20) {
        if (clicked->mode == WINDOW_MODE_FOCUSED) {
            gui_set_window_mode(clicked, WINDOW_MODE_DRAGGABLE);
        } else {
            gui_set_window_mode(clicked, WINDOW_MODE_FOCUSED);
        }
        return;
    }

    /* Check title bar for drag initiation (draggable/overlay modes) */
    if (clicked->mode != WINDOW_MODE_FOCUSED && point_in_title_bar(clicked, mx, my)) {
        clicked->dragging = true;
        clicked->drag_offset_x = mx - clicked->x;
        clicked->drag_offset_y = my - clicked->y;
        dragging_window = clicked;
        return;
    }

    /* Pass click to window's on_click handler */
    if (clicked->on_click) {
        clicked->on_click(clicked, mx, my);
    }
}

/* Handle mouse movement while button is held */
static void handle_mouse_drag(int mx, int my) {
    if (!dragging_window || !dragging_window->dragging) return;

    dragging_window->x = mx - dragging_window->drag_offset_x;
    dragging_window->y = my - dragging_window->drag_offset_y;
    clamp_window_position(dragging_window);
}

/* Handle mouse button release */
static void handle_mouse_release(void) {
    if (dragging_window) {
        dragging_window->dragging = false;
        dragging_window = NULL;
    }
}

/* Update GUI state */
void gui_update(void) {
    /* Handle keyboard input */
    if (keyboard_has_key()) {
        char key = keyboard_get_key();

        /* In focused mode, Escape key starts exit (double-confirm) */
        if (focused_window && focused_window->mode == WINDOW_MODE_FOCUSED) {
            if (key == 27) { /* Escape */
                if (focused_window->quit_confirm_pending) {
                    gui_set_window_mode(focused_window, WINDOW_MODE_DRAGGABLE);
                    focused_window->quit_confirm_pending = false;
                } else {
                    focused_window->quit_confirm_pending = true;
                }
                return;
            } else {
                focused_window->quit_confirm_pending = false;
            }
        }

        if (focused_window && focused_window->on_key) {
            focused_window->on_key(focused_window, key);
        }
    }

    /* Handle mouse input */
    struct mouse_state mouse;
    mouse_get_state(&mouse);

    if (mouse.left_button && !last_left_button) {
        /* Mouse button just pressed */
        handle_mouse_press(mouse.x, mouse.y);
    } else if (mouse.left_button && last_left_button) {
        /* Mouse button held - drag */
        handle_mouse_drag(mouse.x, mouse.y);
    } else if (!mouse.left_button && last_left_button) {
        /* Mouse button released */
        handle_mouse_release();
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

    /* Render windows back to front (sorted by z_order) */
    for (int i = 0; i < window_count; i++) {
        if (!windows[i]->visible) continue;

        gui_draw_window_frame(windows[i]);

        if (windows[i]->render) {
            windows[i]->render(windows[i]);
        }
    }

    /* Draw top bar (always on top) */
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
