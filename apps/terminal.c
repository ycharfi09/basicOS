#include "apps.h"
#include "../gui/include/gui.h"
#include "../drivers/include/framebuffer.h"
#include "../kernel/include/memory.h"
#include "../kernel/include/vfs.h"

/* Terminal data */
#define MAX_LINES 20
#define MAX_LINE_LEN 80
#define MAX_CMD_LEN 64

typedef struct {
    char lines[MAX_LINES][MAX_LINE_LEN];
    int line_count;
    char current_cmd[MAX_CMD_LEN];
    int cmd_pos;
    char cwd[256];
} terminal_data_t;

/* String functions */
static int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

static int strncmp(const char *s1, const char *s2, int n) {
    while (n-- > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return n < 0 ? 0 : *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

static void strcpy(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

/* Add line to terminal */
static void add_line(terminal_data_t *data, const char *line) {
    if (data->line_count >= MAX_LINES) {
        /* Scroll up */
        for (int i = 0; i < MAX_LINES - 1; i++) {
            strcpy(data->lines[i], data->lines[i + 1]);
        }
        data->line_count = MAX_LINES - 1;
    }
    
    strcpy(data->lines[data->line_count++], line);
}

/* Execute terminal command */
static void execute_command(terminal_data_t *data) {
    char output[MAX_LINE_LEN];
    
    if (strcmp(data->current_cmd, "help") == 0) {
        add_line(data, "Available commands:");
        add_line(data, "  help    - Show this help");
        add_line(data, "  ls      - List directory");
        add_line(data, "  cat     - Display file");
        add_line(data, "  echo    - Echo text");
        add_line(data, "  clear   - Clear screen");
        add_line(data, "  pwd     - Print working dir");
        add_line(data, "  uname   - System info");
    } else if (strcmp(data->current_cmd, "ls") == 0) {
        vfs_dirent_t entries[32];
        int count = vfs_list_directory(data->cwd, entries, 32);
        
        if (count > 0) {
            for (int i = 0; i < count; i++) {
                if (entries[i].is_directory) {
                    strcpy(output, "[DIR]  ");
                } else {
                    strcpy(output, "[FILE] ");
                }
                
                /* Append filename (truncate if too long) */
                int j = 7;
                for (int k = 0; entries[i].name[k] && j < MAX_LINE_LEN - 1; k++) {
                    output[j++] = entries[i].name[k];
                }
                output[j] = '\0';
                add_line(data, output);
            }
        } else {
            add_line(data, "No files found or disk not available");
        }
    } else if (strncmp(data->current_cmd, "cat ", 4) == 0) {
        char *filename = data->current_cmd + 4;
        if (vfs_exists(filename)) {
            uint32_t size = vfs_file_size(filename);
            if (size > 0 && size < 1024) {
                add_line(data, "[File content would appear here]");
                /* TODO: Read and display file content */
            } else {
                add_line(data, "File too large or empty");
            }
        } else {
            add_line(data, "File not found");
        }
    } else if (strncmp(data->current_cmd, "echo ", 5) == 0) {
        add_line(data, data->current_cmd + 5);
    } else if (strcmp(data->current_cmd, "clear") == 0) {
        data->line_count = 0;
    } else if (strcmp(data->current_cmd, "pwd") == 0) {
        add_line(data, data->cwd);
    } else if (strcmp(data->current_cmd, "uname") == 0) {
        add_line(data, "BasicOS v2.0 x86_64");
        add_line(data, "Daily Driver Edition");
    } else if (data->current_cmd[0] != '\0') {
        strcpy(output, "Unknown command: ");
        int i = 17;
        for (int j = 0; data->current_cmd[j] && i < MAX_LINE_LEN - 1; j++) {
            output[i++] = data->current_cmd[j];
        }
        output[i] = '\0';
        add_line(data, output);
    }
}

/* Terminal render function */
static void terminal_render(window_t *win) {
    terminal_data_t *data = (terminal_data_t *)win->data;
    
    /* Draw lines */
    int y = 40;
    for (int i = 0; i < data->line_count && y < win->height - 40; i++) {
        fb_draw_string(win->x + 10, win->y + y, data->lines[i], COLOR_WHITE);
        y += 16;
    }
    
    /* Draw prompt with current command */
    char prompt[MAX_LINE_LEN + 4];
    prompt[0] = '>';
    prompt[1] = ' ';
    int pos = 2;
    for (int i = 0; i < data->cmd_pos; i++) {
        prompt[pos++] = data->current_cmd[i];
    }
    prompt[pos++] = '_';
    prompt[pos] = '\0';
    
    fb_draw_string(win->x + 10, win->y + win->height - 30, prompt, COLOR_GREEN);
}

/* Terminal key handler */
static void terminal_on_key(window_t *win, char key) {
    terminal_data_t *data = (terminal_data_t *)win->data;
    
    if (key == '\n') {
        /* Execute command */
        data->current_cmd[data->cmd_pos] = '\0';
        
        /* Echo command */
        char echo[MAX_LINE_LEN];
        echo[0] = '>';
        echo[1] = ' ';
        int pos = 2;
        for (int i = 0; i < data->cmd_pos && pos < MAX_LINE_LEN - 1; i++) {
            echo[pos++] = data->current_cmd[i];
        }
        echo[pos] = '\0';
        add_line(data, echo);
        
        /* Execute */
        execute_command(data);
        
        /* Reset command */
        data->cmd_pos = 0;
        data->current_cmd[0] = '\0';
    } else if (key == '\b') {
        /* Backspace */
        if (data->cmd_pos > 0) {
            data->cmd_pos--;
            data->current_cmd[data->cmd_pos] = '\0';
        }
    } else {
        /* Add character */
        if (data->cmd_pos < MAX_CMD_LEN - 1 && key >= 32 && key <= 126) {
            data->current_cmd[data->cmd_pos++] = key;
            data->current_cmd[data->cmd_pos] = '\0';
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
    data->line_count = 0;
    data->cmd_pos = 0;
    data->current_cmd[0] = '\0';
    strcpy(data->cwd, "/");
    
    /* Welcome message */
    add_line(data, "BasicOS Terminal v2.0");
    add_line(data, "Type 'help' for available commands");
    add_line(data, "");

    win->data = data;
    win->bg_color = RGB(0, 0, 0);
    win->render = terminal_render;
    win->on_key = terminal_on_key;
}
