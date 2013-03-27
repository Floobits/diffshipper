#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "conf.h"
#include "log.h"
#include "options.h"
#include "util.h"


static char *parse_value(char *value) {
    char *end;
    while (isspace(*value)) {
        value++;
    }
    end = value;
    while (!isspace(*end), *end != '\0') {
        end++;
    }
    *end = '\0';

    return strdup(value);
}

int parse_conf() {
    int i;
    FILE *fp = NULL;
    char *line = NULL;
    size_t line_cap;
    ssize_t line_len;
    char *path = NULL;
    char *home = getenv("HOME");
    int rv = 0;

    if (!home) {
        log_err("HOME environment variable isn't set. Not parsing config.");
        goto cleanup;
    }

    ds_asprintf(&path, "%s/%s", home, FLOORC_FILE);

    fp = fopen(path, "r");
    if (!fp) {
        log_err("Error opening floorc file %s: %s", path, strerror(errno));
        goto cleanup;
    }

    for (i = 0; (line_len = getline(&line, &line_cap, fp)) > 0; i++) {
        if (line_len == 0 || line[0] == '\n' || line[0] == '#') {
            continue;
        }
        if (line[line_len-1] == '\n') {
            line[line_len-1] = '\0'; /* kill the \n */
        }

        if (strncmp(line, "username ", 9) == 0) {
            opts.username = parse_value(line + 9);
            if (!opts.username) {
                log_err("Couldn't parse username on line %i in %s", i, path);
                rv = -1;
                goto cleanup;
            }
        } else if (strncmp(line, "secret ", 7) == 0) {
            opts.secret = parse_value(line + 7);
            if (!opts.secret) {
                log_err("Couldn't parse secret on line %i in %s", i, path);
                rv = -1;
                goto cleanup;
            }
        }
    }

    cleanup:;
    if (fp)
        fclose(fp);
    if (line)
        free(line);
    if (path)
        free(path);

    return rv;
}
