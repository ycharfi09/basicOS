#include "apps.h"
#include "../gui/include/gui.h"
#include "../drivers/include/framebuffer.h"
#include "../kernel/include/memory.h"

/* Terminal data */
typedef struct {
    char text[1024];
    int cursor;
} terminal_data_t;

/* Terminal render function */
static void terminal_render(window_t *win) {
    terminal_data_t *data = (terminal_data_t *)win->data;
    
    /* Draw text */
    fb_draw_string(win->x + 10, win->y + 40, data->text, COLOR_WHITE);
    
    /* Draw prompt */
    fb_draw_string(win->x + 10, win->y + win->height - 40, "> _", COLOR_GREEN);
}

/* Terminal key handler */
static void terminal_on_key(window_t *win, char key) {
    terminal_data_t *data = (terminal_data_t *)win->data;
    
    if (key == '\n') {
        /* Add newline */
        if (data->cursor < 1000) {
            data->text[data->cursor++] = '\n';
            data->text[data->cursor++] = '>';
            data->text[data->cursor++] = ' ';
            data->text[data->cursor] = '\0';
        }
    } else if (key == '\b') {
        /* Backspace */
        if (data->cursor > 0) {
            data->cursor--;
            data->text[data->cursor] = '\0';
        }
    } else {
        /* Add character */
        if (data->cursor < 1023) {
            data->text[data->cursor++] = key;
            data->text[data->cursor] = '\0';
        }
    }
}

/* Create terminal app */
void app_terminal_create(void) {
    window_t *win = gui_create_window("Terminal", 100, 100, 600, 400);
    if (!win) return;

    terminal_data_t *data = (terminal_data_t *)kmalloc(sizeof(terminal_data_t));
    if (!data) return;

    /* Initialize terminal */
    data->text[0] = '>';
    data->text[1] = ' ';
    data->text[2] = '\0';
    data->cursor = 2;

    win->data = data;
    win->bg_color = RGB(0, 0, 0);
    win->render = terminal_render;
    win->on_key = terminal_on_key;
}
