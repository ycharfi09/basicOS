#include "apps.h"
#include "../gui/include/gui.h"
#include "../drivers/include/framebuffer.h"
#include "../kernel/include/memory.h"

/* Text editor data */
#define EDITOR_MAX_TEXT 4096
#define EDITOR_LINE_HEIGHT 12

typedef struct {
    char text[EDITOR_MAX_TEXT];
    int length;
    int cursor;          /* Cursor position in text buffer */
    int scroll_y;        /* Vertical scroll offset in lines */
    bool modified;       /* Has unsaved changes */
    char filename[64];   /* Current filename */
} editor_data_t;

/* Count lines up to a given position */
static int editor_count_lines(const char *text, int pos) {
    int lines = 0;
    for (int i = 0; i < pos; i++) {
        if (text[i] == '\n') lines++;
    }
    return lines;
}

/* Get column of cursor in current line */
static int editor_get_col(const char *text, int pos) {
    int col = 0;
    for (int i = pos - 1; i >= 0; i--) {
        if (text[i] == '\n') break;
        col++;
    }
    return col;
}

/* Editor render function */
static void editor_render(window_t *win) {
    editor_data_t *data = (editor_data_t *)win->data;

    int content_x = win->x + 10;
    int content_y = win->y + 40;
    int content_h = win->height - 60;
    int visible_lines = content_h / EDITOR_LINE_HEIGHT;
    if (visible_lines < 1) visible_lines = 1;

    /* Draw text with line numbers */
    int line = 0;
    int col = 0;
    int cursor_draw_x = content_x;
    int cursor_draw_y = content_y;

    for (int i = 0; i <= data->length; i++) {
        int display_line = line - data->scroll_y;

        /* Track cursor position for rendering */
        if (i == data->cursor) {
            cursor_draw_x = content_x + 30 + col * 8;
            cursor_draw_y = content_y + display_line * EDITOR_LINE_HEIGHT;
        }

        if (i == data->length) break;

        if (data->text[i] == '\n') {
            line++;
            col = 0;
        } else {
            /* Draw character if visible */
            if (display_line >= 0 && display_line < visible_lines) {
                fb_draw_char(content_x + 30 + col * 8,
                             content_y + display_line * EDITOR_LINE_HEIGHT,
                             data->text[i], COLOR_BLACK);
            }
            col++;
        }
    }

    /* Draw line numbers for visible lines */
    int total_lines = editor_count_lines(data->text, data->length) + 1;
    for (int l = 0; l < visible_lines && (l + data->scroll_y) < total_lines; l++) {
        char num[4];
        int n = l + data->scroll_y + 1;
        num[0] = (n / 10) ? ('0' + (n / 10) % 10) : ' ';
        num[1] = '0' + n % 10;
        num[2] = '\0';
        fb_draw_string(content_x, content_y + l * EDITOR_LINE_HEIGHT, num, COLOR_GRAY);
    }

    /* Draw cursor */
    int cursor_display_line = editor_count_lines(data->text, data->cursor) - data->scroll_y;
    if (cursor_display_line >= 0 && cursor_display_line < visible_lines) {
        fb_draw_rect(cursor_draw_x, cursor_draw_y, 2, EDITOR_LINE_HEIGHT, COLOR_BLACK);
    }

    /* Status bar */
    int status_y = win->y + win->height - 18;
    fb_draw_rect(win->x, status_y, win->width, 18, RGB(200, 200, 200));
    char status[80];
    int line_num = editor_count_lines(data->text, data->cursor) + 1;
    int col_num = editor_get_col(data->text, data->cursor) + 1;

    /* Build status string manually */
    int si = 0;
    const char *lbl = "Ln ";
    while (*lbl) status[si++] = *lbl++;
    if (line_num >= 10) status[si++] = '0' + (line_num / 10) % 10;
    status[si++] = '0' + line_num % 10;
    status[si++] = ' ';
    const char *clbl = "Col ";
    while (*clbl) status[si++] = *clbl++;
    if (col_num >= 10) status[si++] = '0' + (col_num / 10) % 10;
    status[si++] = '0' + col_num % 10;
    if (data->modified) {
        const char *mod = " [modified]";
        while (*mod) status[si++] = *mod++;
    }
    status[si] = '\0';

    fb_draw_string(win->x + 10, status_y + 5, status, COLOR_BLACK);
}

/* Editor key handler */
static void editor_on_key(window_t *win, char key) {
    editor_data_t *data = (editor_data_t *)win->data;

    if (key == '\b') {
        /* Backspace: delete character before cursor */
        if (data->cursor > 0) {
            /* Shift text left */
            for (int i = data->cursor - 1; i < data->length - 1; i++) {
                data->text[i] = data->text[i + 1];
            }
            data->cursor--;
            data->length--;
            data->text[data->length] = '\0';
            data->modified = true;
        }
    } else if (key == 127) {
        /* Delete key: delete character at cursor */
        if (data->cursor < data->length) {
            for (int i = data->cursor; i < data->length - 1; i++) {
                data->text[i] = data->text[i + 1];
            }
            data->length--;
            data->text[data->length] = '\0';
            data->modified = true;
        }
    } else if (key == 2) {
        /* Ctrl+B or left arrow: move cursor left */
        if (data->cursor > 0) data->cursor--;
    } else if (key == 6) {
        /* Ctrl+F or right arrow: move cursor right */
        if (data->cursor < data->length) data->cursor++;
    } else if (key == 16) {
        /* Ctrl+P: move cursor up one line */
        int col = editor_get_col(data->text, data->cursor);
        /* Find start of current line */
        int pos = data->cursor;
        while (pos > 0 && data->text[pos - 1] != '\n') pos--;
        /* Move to previous line */
        if (pos > 0) {
            pos--; /* skip newline */
            int prev_start = pos;
            while (prev_start > 0 && data->text[prev_start - 1] != '\n') prev_start--;
            int prev_len = pos - prev_start;
            data->cursor = prev_start + (col < prev_len ? col : prev_len);
        }
    } else if (key == 15) {
        /* Ctrl+O: move cursor down one line */
        int col = editor_get_col(data->text, data->cursor);
        /* Find end of current line */
        int pos = data->cursor;
        while (pos < data->length && data->text[pos] != '\n') pos++;
        /* Move to next line */
        if (pos < data->length) {
            pos++; /* skip newline */
            int next_end = pos;
            while (next_end < data->length && data->text[next_end] != '\n') next_end++;
            int next_len = next_end - pos;
            data->cursor = pos + (col < next_len ? col : next_len);
        }
    } else if (key == 1) {
        /* Ctrl+A: move to start of line */
        while (data->cursor > 0 && data->text[data->cursor - 1] != '\n') {
            data->cursor--;
        }
    } else if (key == 5) {
        /* Ctrl+E: move to end of line */
        while (data->cursor < data->length && data->text[data->cursor] != '\n') {
            data->cursor++;
        }
    } else {
        /* Insert character at cursor position */
        if (data->length < EDITOR_MAX_TEXT - 1 && ((key >= 32 && key <= 126) || key == '\n' || key == '\t')) {
            /* Shift text right */
            for (int i = data->length; i > data->cursor; i--) {
                data->text[i] = data->text[i - 1];
            }
            data->text[data->cursor] = key;
            data->cursor++;
            data->length++;
            data->text[data->length] = '\0';
            data->modified = true;
        }
    }

    /* Auto-scroll to keep cursor visible */
    int cursor_line = editor_count_lines(data->text, data->cursor);
    int visible_lines = (win->height - 60) / EDITOR_LINE_HEIGHT;
    if (visible_lines < 1) visible_lines = 1;
    if (cursor_line < data->scroll_y) {
        data->scroll_y = cursor_line;
    }
    if (cursor_line >= data->scroll_y + visible_lines) {
        data->scroll_y = cursor_line - visible_lines + 1;
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
    data->length = 0;
    data->cursor = 0;
    data->scroll_y = 0;
    data->modified = false;
    data->filename[0] = '\0';

    win->data = data;
    win->render = editor_render;
    win->on_key = editor_on_key;
}
