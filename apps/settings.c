#include "apps.h"
#include "../gui/include/gui.h"
#include "../drivers/include/framebuffer.h"
#include "../kernel/include/memory.h"

/* Settings data */
typedef struct {
    int selected_color;
    bool saved;           /* Whether current settings have been saved */
    char status[64];      /* Status message */
} settings_data_t;

/* Color scheme definitions */
static const uint32_t scheme_colors[] = {
    0x00F0F0F0,  /* Light (RGB(240,240,240)) */
    0x003C3C3C,  /* Dark  (RGB(60,60,60))    */
    0x00C8DCFF,  /* Blue  (RGB(200,220,255)) */
    0x00FFF0C8   /* Warm  (RGB(255,240,200)) */
};

static const char *scheme_names[] = {"Light", "Dark", "Blue", "Warm"};

static void settings_strcpy(char *dest, const char *src) {
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

/* Settings render function */
static void settings_render(window_t *win) {
    settings_data_t *data = (settings_data_t *)win->data;

    /* Draw title */
    fb_draw_string(win->x + 10, win->y + 40, "Settings", COLOR_BLACK);

    /* Draw color options */
    fb_draw_string(win->x + 10, win->y + 80, "Color Scheme:", COLOR_BLACK);

    /* Color buttons */
    for (int i = 0; i < 4; i++) {
        int button_y = win->y + 110 + i * 40;
        bool selected = (data->selected_color == i);

        gui_draw_button(win->x + 20, button_y, 150, 30, scheme_names[i], selected);

        /* Color preview */
        fb_draw_rect(win->x + 180, button_y + 5, 30, 20, scheme_colors[i]);
    }

    /* Save button */
    gui_draw_button(win->x + 20, win->y + 280, 100, 30, "Save", false);

    /* Status message */
    if (data->status[0]) {
        uint32_t msg_color = data->saved ? RGB(0, 128, 0) : COLOR_GRAY;
        fb_draw_string(win->x + 130, win->y + 288, data->status, msg_color);
    }
}

/* Settings click handler */
static void settings_on_click(window_t *win, int x, int y) {
    settings_data_t *data = (settings_data_t *)win->data;

    /* Check color button clicks */
    for (int i = 0; i < 4; i++) {
        int button_y = win->y + 110 + i * 40;
        if (x >= win->x + 20 && x <= win->x + 170 &&
            y >= button_y && y <= button_y + 30) {
            data->selected_color = i;
            data->saved = false;
            settings_strcpy(data->status, "");
            break;
        }
    }

    /* Check save button click */
    if (x >= win->x + 20 && x <= win->x + 120 &&
        y >= win->y + 280 && y <= win->y + 310) {
        data->saved = true;
        settings_strcpy(data->status, "Saved!");
        /* Settings saved (in-memory; disk persistence needs filesystem support) */
    }
}

/* Create settings app */
void app_settings_create(void) {
    window_t *win = gui_create_window("Settings", 200, 150, 300, 350);
    if (!win) return;

    settings_data_t *data = (settings_data_t *)kmalloc(sizeof(settings_data_t));
    if (!data) return;

    /* Initialize settings */
    data->selected_color = 0;
    data->saved = false;
    data->status[0] = '\0';

    win->data = data;
    win->render = settings_render;
    win->on_click = settings_on_click;
}
