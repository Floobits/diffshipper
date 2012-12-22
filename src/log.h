#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

#include <jansson.h>

enum log_level {
    LOG_LEVEL_DEBUG = 10,
    LOG_LEVEL_MSG   = 20,
    LOG_LEVEL_WARN  = 30,
    LOG_LEVEL_ERR   = 40
};

void set_log_level(enum log_level threshold);

void log_debug(const char *fmt, ...);
void log_msg(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_err(const char *fmt, ...);

void die(const char *fmt, ...);

void log_json_err(json_error_t *err);

#endif
