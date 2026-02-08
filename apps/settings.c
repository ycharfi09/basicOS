#include "apps.h"
#include "../gui/include/gui.h"
#include "../drivers/include/framebuffer.h"
#include "../kernel/include/memory.h"

/* Settings data */
typedef struct {
    int selected_color;
} settings_data_t;

/* Settings render function */
static void settings_render(window_t *win) {
    settings_data_t *data = (settings_data_t *)win->data;
    
    /* Draw title */
    fb_draw_string(win->x + 10, win->y + 40, "Settings", COLOR_BLACK);
    
    /* Draw color options */
    fb_draw_string(win->x + 10, win->y + 80, "Color Scheme:", COLOR_BLACK);
    
    /* Color buttons */
    uint32_t colors[] = {
        RGB(240, 240, 240),  /* Light */
        RGB(60, 60, 60),     /* Dark */
        RGB(200, 220, 255),  /* Blue */
        RGB(255, 240, 200)   /* Warm */
    };
    
    const char *color_names[] = {"Light", "Dark", "Blue", "Warm"};
    
    for (int i = 0; i < 4; i++) {
        int button_y = win->y + 110 + i * 40;
        bool selected = (data->selected_color == i);
        
        gui_draw_button(win->x + 20, button_y, 150, 30, color_names[i], selected);
        
        /* Color preview */
        fb_draw_rect(win->x + 180, button_y + 5, 30, 20, colors[i]);
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
            break;
        }
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

    win->data = data;
    win->render = settings_render;
    win->on_click = settings_on_click;
}
