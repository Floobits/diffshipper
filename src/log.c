#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.h"


static enum log_level log_threshold = LOG_LEVEL_ERR;


static void vplog(const unsigned int level, const char *fmt, va_list args) {
    if (level < log_threshold) {
        return;
    }

    FILE *stream = stdout;

    switch(level) {
        case LOG_LEVEL_DEBUG:
            fprintf(stream, "DEBUG: ");
        break;
        case LOG_LEVEL_MSG:
            fprintf(stream, "MSG: ");
        break;
        case LOG_LEVEL_WARN:
            fprintf(stream, "WARN: ");
        break;
        case LOG_LEVEL_ERR:
            stream = stderr;
            fprintf(stream, "ERR: ");
        break;
    }

    vfprintf(stream, fmt, args);
    fprintf(stream, "\n");
}


void set_log_level(enum log_level threshold) {
    log_threshold = threshold;
}


/* Maybe these should be macros? */
void log_debug(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vplog(LOG_LEVEL_DEBUG, fmt, args);
    va_end(args);
}


void log_msg(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vplog(LOG_LEVEL_MSG, fmt, args);
    va_end(args);
}


void log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vplog(LOG_LEVEL_WARN, fmt, args);
    va_end(args);
}


void log_err(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vplog(LOG_LEVEL_ERR, fmt, args);
    va_end(args);
}


void die(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vplog(LOG_LEVEL_ERR, fmt, args);
    va_end(args);
    exit(1);
}


void log_json_err(json_error_t *err) {
    log_msg("JSON error: source %s, line %i column %i (offset %u): %s", err->source, err->line, err->column, err->position, err->text);
}
