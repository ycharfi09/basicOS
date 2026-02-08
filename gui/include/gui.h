#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include <stdbool.h>

/* Window modes */
typedef enum {
    WINDOW_MODE_FOCUSED,    /* Fullscreen with zoom animation */
    WINDOW_MODE_DRAGGABLE,  /* Floating window */
    WINDOW_MODE_OVERLAY     /* Overlapping windows */
} window_mode_t;

/* Window structure */
typedef struct window {
    int x, y;
    int width, height;
    char title[64];
    uint32_t bg_color;
    window_mode_t mode;
    bool visible;
    bool focused;
    void (*render)(struct window *win);
    void (*update)(struct window *win);
    void (*on_key)(struct window *win, char key);
    void (*on_click)(struct window *win, int x, int y);
    void *data;  /* Application-specific data */
} window_t;

/* GUI functions */
void gui_init(void);
void gui_update(void);
void gui_render(void);
window_t *gui_create_window(const char *title, int x, int y, int width, int height);
void gui_close_window(window_t *win);
void gui_focus_window(window_t *win);
void gui_set_window_mode(window_t *win, window_mode_t mode);

/* Drawing helpers */
void gui_draw_window_frame(window_t *win);
void gui_draw_button(int x, int y, int width, int height, const char *text, bool pressed);

#endif /* GUI_H */
