#include "apps.h"
#include "../gui/include/gui.h"
#include "../drivers/include/framebuffer.h"
#include "../kernel/include/memory.h"
#include "../kernel/include/vfs.h"

/* File manager data */
#define FM_MAX_ENTRIES 32

typedef struct {
    char cwd[256];
    vfs_dirent_t entries[FM_MAX_ENTRIES];
    int entry_count;
    int selected;           /* Currently selected entry index */
    int scroll_offset;      /* Scroll offset for long listings */
    char status[80];        /* Status bar message */
} files_data_t;

/* String helpers */
static void fm_strcpy(char *dest, const char *src) {
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

/* Refresh directory listing */
static void fm_refresh(files_data_t *data) {
    data->entry_count = vfs_list_directory(data->cwd, data->entries, FM_MAX_ENTRIES);
    if (data->entry_count < 0) {
        data->entry_count = 0;
        fm_strcpy(data->status, "No disk or filesystem found");
    } else {
        fm_strcpy(data->status, data->cwd);
    }
    data->selected = 0;
    data->scroll_offset = 0;
}

/* File manager render function */
static void files_render(window_t *win) {
    files_data_t *data = (files_data_t *)win->data;

    /* Toolbar area */
    fb_draw_rect(win->x + 2, win->y + 32, win->width - 4, 24, RGB(220, 220, 220));
    fb_draw_string(win->x + 10, win->y + 38, data->cwd, COLOR_BLACK);

    /* File list area */
    int list_y = win->y + 60;
    int list_h = win->height - 90;
    int visible = list_h / 20;
    if (visible < 1) visible = 1;

    /* Column headers */
    fb_draw_string(win->x + 10, list_y, "Type", COLOR_GRAY);
    fb_draw_string(win->x + 80, list_y, "Name", COLOR_GRAY);
    fb_draw_string(win->x + win->width - 80, list_y, "Size", COLOR_GRAY);
    list_y += 20;

    if (data->entry_count == 0) {
        fb_draw_string(win->x + 10, list_y, "No files found", COLOR_GRAY);
    } else {
        for (int i = data->scroll_offset;
             i < data->entry_count && (i - data->scroll_offset) < visible;
             i++) {
            int row_y = list_y + (i - data->scroll_offset) * 20;
            bool is_sel = (i == data->selected);

            /* Highlight selected row */
            if (is_sel) {
                fb_draw_rect(win->x + 4, row_y - 2, win->width - 8, 18, RGB(180, 210, 255));
            }

            /* Type indicator */
            if (data->entries[i].is_directory) {
                fb_draw_string(win->x + 10, row_y, "[DIR]", COLOR_BLUE);
            } else {
                fb_draw_string(win->x + 10, row_y, "[FIL]", COLOR_BLACK);
            }

            /* Name */
            uint32_t name_color = is_sel ? RGB(0, 0, 128) : COLOR_BLACK;
            fb_draw_string(win->x + 80, row_y, data->entries[i].name, name_color);

            /* Size (for files) */
            if (!data->entries[i].is_directory) {
                char size_str[16];
                uint32_t sz = data->entries[i].size;
                int si = 0;
                if (sz == 0) {
                    size_str[si++] = '0';
                } else {
                    char tmp[16];
                    int ti = 0;
                    while (sz > 0) {
                        tmp[ti++] = '0' + sz % 10;
                        sz /= 10;
                    }
                    while (ti > 0) size_str[si++] = tmp[--ti];
                }
                size_str[si++] = 'B';
                size_str[si] = '\0';
                fb_draw_string(win->x + win->width - 80, row_y, size_str, COLOR_GRAY);
            }
        }
    }

    /* Status bar */
    int status_y = win->y + win->height - 22;
    fb_draw_rect(win->x, status_y, win->width, 22, RGB(200, 200, 200));
    fb_draw_string(win->x + 10, status_y + 5, data->status, COLOR_BLACK);
}

/* File manager click handler */
static void files_on_click(window_t *win, int x, int y) {
    files_data_t *data = (files_data_t *)win->data;
    (void)x;

    int list_y = win->y + 80; /* After header row */
    int row_index = (y - list_y) / 20 + data->scroll_offset;
    if (row_index >= 0 && row_index < data->entry_count) {
        data->selected = row_index;
    }
}

/* File manager key handler */
static void files_on_key(window_t *win, char key) {
    files_data_t *data = (files_data_t *)win->data;
    (void)win;

    if (key == 16) {
        /* Up */
        if (data->selected > 0) data->selected--;
    } else if (key == 15) {
        /* Down */
        if (data->selected < data->entry_count - 1) data->selected++;
    } else if (key == '\n') {
        /* Enter: open directory or show file info */
        if (data->selected >= 0 && data->selected < data->entry_count) {
            if (data->entries[data->selected].is_directory) {
                /* Navigate into directory */
                fm_strcpy(data->status, "Opened: ");
                /* Append directory name to status */
                int j = 8;
                for (int k = 0; data->entries[data->selected].name[k] && j < 79; k++) {
                    data->status[j++] = data->entries[data->selected].name[k];
                }
                data->status[j] = '\0';
            } else {
                fm_strcpy(data->status, "File: ");
                int j = 6;
                for (int k = 0; data->entries[data->selected].name[k] && j < 79; k++) {
                    data->status[j++] = data->entries[data->selected].name[k];
                }
                data->status[j] = '\0';
            }
        }
    } else if (key == 'r' || key == 'R') {
        /* Refresh */
        fm_refresh(data);
    }
}

/* Create file manager app */
void app_files_create(void) {
    window_t *win = gui_create_window("Files", 250, 200, 450, 380);
    if (!win) return;

    files_data_t *data = (files_data_t *)kmalloc(sizeof(files_data_t));
    if (!data) return;

    fm_strcpy(data->cwd, "/");
    data->selected = 0;
    data->scroll_offset = 0;
    fm_strcpy(data->status, "/");
    fm_refresh(data);

    win->data = data;
    win->render = files_render;
    win->on_click = files_on_click;
    win->on_key = files_on_key;
}
