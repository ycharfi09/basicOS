#include "apps.h"
#include "../gui/include/gui.h"
#include "../drivers/include/framebuffer.h"

/* File manager render function */
static void files_render(window_t *win) {
    /* Draw title */
    fb_draw_string(win->x + 10, win->y + 40, "File Manager", COLOR_BLACK);
    
    /* Draw fake file list */
    const char *files[] = {
        "[DIR] /",
        "[DIR] boot",
        "[DIR] kernel",
        "[DIR] drivers",
        "[FILE] README.txt",
        "[FILE] config.sys"
    };
    
    for (int i = 0; i < 6; i++) {
        fb_draw_string(win->x + 20, win->y + 80 + i * 20, files[i], COLOR_BLACK);
    }
}

/* Create file manager app */
void app_files_create(void) {
    window_t *win = gui_create_window("Files", 250, 200, 400, 350);
    if (!win) return;

    win->render = files_render;
}
