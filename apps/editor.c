#include "apps.h"
#include "../gui/include/gui.h"
#include "../drivers/include/framebuffer.h"
#include "../kernel/include/memory.h"

/* Text editor data */
typedef struct {
    char text[2048];
    int cursor;
} editor_data_t;

/* Editor render function */
static void editor_render(window_t *win) {
    editor_data_t *data = (editor_data_t *)win->data;
    
    /* Draw text */
    fb_draw_string(win->x + 10, win->y + 40, data->text, COLOR_BLACK);
    
    /* Draw cursor indicator */
    fb_draw_string(win->x + 10, win->y + win->height - 30, "Press keys to type", COLOR_GRAY);
}

/* Editor key handler */
static void editor_on_key(window_t *win, char key) {
    editor_data_t *data = (editor_data_t *)win->data;
    
    if (key == '\b') {
        /* Backspace */
        if (data->cursor > 0) {
            data->cursor--;
            data->text[data->cursor] = '\0';
        }
    } else {
        /* Add character */
        if (data->cursor < 2047) {
            data->text[data->cursor++] = key;
            data->text[data->cursor] = '\0';
        }
    }
}

/* Create text editor app */
void app_editor_create(void) {
    window_t *win = gui_create_window("Text Editor", 150, 150, 500, 400);
    if (!win) return;

    editor_data_t *data = (editor_data_t *)kmalloc(sizeof(editor_data_t));
    if (!data) return;

    /* Initialize editor */
    data->text[0] = '\0';
    data->cursor = 0;

    win->data = data;
    win->render = editor_render;
    win->on_key = editor_on_key;
}
