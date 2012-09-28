#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "log.h"
#include "util.h"


int run_cmd(const char *fmt, ...) {
    char *cmd;
    int rv;
    va_list args;

    va_start(args, fmt);
    vasprintf(&cmd, fmt, args);
    va_end(args);

    log_debug("Running %s", cmd);
    rv = system(cmd);

    return rv;
}
