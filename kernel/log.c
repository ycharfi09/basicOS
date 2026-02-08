#include "log.h"
#include "../../drivers/include/framebuffer.h"
#include <stdarg.h>
#include <stdbool.h>

/* Log buffer */
#define LOG_BUFFER_SIZE 4096
static char log_buffer[LOG_BUFFER_SIZE];
static int log_offset = 0;
static bool log_initialized = false;

/* String functions */
static void strcpy(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

/* Initialize logging */
void log_init(void) {
    log_offset = 0;
    log_buffer[0] = '\0';
    log_initialized = true;
}

/* Get log level string */
static const char *get_level_str(log_level_t level) {
    switch (level) {
        case LOG_DEBUG: return "[DEBUG]";
        case LOG_INFO:  return "[INFO] ";
        case LOG_WARN:  return "[WARN] ";
        case LOG_ERROR: return "[ERROR]";
        default:        return "[?????]";
    }
}

/* Log a message */
void log_message(log_level_t level, const char *subsystem, const char *message) {
    if (!log_initialized) return;
    
    /* Build log entry */
    char entry[256];
    int pos = 0;
    
    /* Add level */
    const char *level_str = get_level_str(level);
    for (int i = 0; level_str[i] && pos < 248; i++) {
        entry[pos++] = level_str[i];
    }
    if (pos < 248) entry[pos++] = ' ';
    
    /* Add subsystem */
    if (subsystem && pos < 246) {
        entry[pos++] = '[';
        for (int i = 0; subsystem[i] && pos < 245; i++) {
            entry[pos++] = subsystem[i];
        }
        if (pos < 246) entry[pos++] = ']';
        if (pos < 247) entry[pos++] = ' ';
    }
    
    /* Add message */
    for (int i = 0; message[i] && pos < 255; i++) {
        entry[pos++] = message[i];
    }
    entry[pos] = '\0';
    
    /* Add to buffer */
    int entry_len = 0;
    while (entry[entry_len]) entry_len++;
    
    if (log_offset + entry_len + 2 < LOG_BUFFER_SIZE) {
        strcpy(log_buffer + log_offset, entry);
        log_offset += entry_len;
        log_buffer[log_offset++] = '\n';
        log_buffer[log_offset] = '\0';
    }
    
    /* Also output to screen for debugging (optional) */
    /* fb_draw_string(10, 50 + log_offset, entry, COLOR_WHITE); */
}

/* Simple printf-like logging (limited functionality) */
void log_printf(log_level_t level, const char *subsystem, const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    
    /* Simple format string handling - only supports %s and %d */
    int pos = 0;
    const char *p = format;
    
    while (*p && pos < 255) {
        if (*p == '%' && *(p + 1)) {
            p++;
            if (*p == 's') {
                const char *str = va_arg(args, const char *);
                while (*str && pos < 255) {
                    buffer[pos++] = *str++;
                }
            } else if (*p == 'd') {
                int num = va_arg(args, int);
                char temp[32];
                int i = 0;
                
                if (num == 0) {
                    buffer[pos++] = '0';
                } else {
                    if (num < 0) {
                        buffer[pos++] = '-';
                        num = -num;
                    }
                    while (num > 0) {
                        temp[i++] = '0' + (num % 10);
                        num /= 10;
                    }
                    while (i > 0 && pos < 255) {
                        buffer[pos++] = temp[--i];
                    }
                }
            }
            p++;
        } else {
            buffer[pos++] = *p++;
        }
    }
    buffer[pos] = '\0';
    
    va_end(args);
    log_message(level, subsystem, buffer);
}
