#ifndef LOG_H
#define LOG_H

/* Log levels */
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

/* Logging functions */
void log_init(void);
void log_message(log_level_t level, const char *subsystem, const char *message);
void log_printf(log_level_t level, const char *subsystem, const char *format, ...);

/* Convenience macros */
#define LOG_DEBUG_MSG(sys, msg) log_message(LOG_DEBUG, sys, msg)
#define LOG_INFO_MSG(sys, msg)  log_message(LOG_INFO, sys, msg)
#define LOG_WARN_MSG(sys, msg)  log_message(LOG_WARN, sys, msg)
#define LOG_ERROR_MSG(sys, msg) log_message(LOG_ERROR, sys, msg)

#endif /* LOG_H */
