#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "md5.h"

#include "log.h"
#include "util.h"


int run_cmd(const char *fmt, ...) {
    char *cmd;
    int rv;
    va_list args;

    va_start(args, fmt);
    if (vasprintf(&cmd, fmt, args) == -1)
        die("vasprintf returned -1");
    va_end(args);

    log_debug("Running %s", cmd);
    rv = system(cmd);

    free(cmd);

    return rv;
}


char *escape_data(const char *data, int len) {
    const char escape_whitelist[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789;,/?:@&=+$-_.!~*'()\0";
    char *escaped;
    int i;
    int offset = 0;
    char *escaped_char;

    escaped = malloc(len + 1);

    for (i = 0; i < len; i++) {
        escaped[i + offset] = data[i];
        if (strchr(escape_whitelist, data[i]) == NULL) {
            ds_asprintf(&escaped_char, "%%%02X", data[i]);
            escaped = realloc(escaped, len + 1 + offset + strlen(escaped_char));
            strcpy(&escaped[i + offset], escaped_char);
            offset += strlen(escaped_char);
            free(escaped_char);
        }
    }
    escaped[len + offset] = '\0';
    log_debug("escaped: %s", escaped);
    return escaped;
}


char *unescape_data(char *escaped) {
    int escaped_len = strlen(escaped);
    int i;
    int offset = 0;
    char escaped_char[3];
    unsigned int unescaped_char;
    log_debug("escaped: '%s'", escaped);
    char *data = strdup(escaped);

    for (i = 0; i < escaped_len; i++) {
        if (escaped[i] == '%') {
            /* we assume it's a valid escaped string */
            strncpy(escaped_char, &escaped[i+1], 3);
            sscanf(escaped_char, "%02X", &unescaped_char);
            data[i - offset] = (char)unescaped_char;
            offset += 2;
            i += 2;
        } else {
            data[i - offset] = escaped[i];
        }
    }

    data[i - offset] = '\0';
    log_debug("unescaped: '%s'", data);
    return data;
}


/* To get around gcc's -Wunused-result */
void ds_asprintf(char **ret, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (vasprintf(ret, fmt, args) == -1)
        die("vasprintf returned -1");
    va_end(args);
}


int is_binary(const void* buf, const int buf_len) {
    int suspicious_bytes = 0;
    int total_bytes = buf_len > 1024 ? 1024 : buf_len;
    const unsigned char *buf_c = buf;
    int i;

    if (buf_len == 0) {
        return 0;
    }

    if (buf_len >= 3 && buf_c[0] == 0xEF && buf_c[1] == 0xBB && buf_c[2] == 0xBF) {
        /* UTF-8 BOM. This isn't binary. */
        return 0;
    }

    for (i = 0; i < total_bytes; i++) {
        /* Disk IO is so slow that it's worthwhile to do this calculation after every suspicious byte. */
        /* This is true even on a 1.6Ghz Atom with an Intel 320 SSD. */
        /* Read at least 32 bytes before making a decision */
        if (i >= 32 && (suspicious_bytes * 100) / total_bytes > 10) {
            return 1;
        }

        if (buf_c[i] == '\0') {
            /* NULL char. It's binary */
            return 1;
        } else if ((buf_c[i] < 7 || buf_c[i] > 14) && (buf_c[i] < 32 || buf_c[i] > 127)) {
            /* UTF-8 detection */
            if (buf_c[i] > 191 && buf_c[i] < 224 && i + 1 < total_bytes) {
                i++;
                if (buf_c[i] < 192) {
                    continue;
                }
            } else if (buf_c[i] > 223 && buf_c[i] < 239 && i + 2 < total_bytes) {
                i++;
                if (buf_c[i] < 192 && buf_c[i + 1] < 192) {
                    i++;
                    continue;
                }
            }
            suspicious_bytes++;
        }
    }
    if ((suspicious_bytes * 100) / total_bytes > 10) {
        return 1;
    }

    return 0;
}


void parse_json(json_t *json_obj, const char *fmt, ...) {
    int rv;
    va_list args;
    json_error_t json_err;

    va_start(args, fmt);
    rv = json_vunpack_ex(json_obj, &json_err, 0, fmt, args);
    va_end(args);

    if (rv) {
        log_json_err(&json_err);
        die("Couldn't parse json.");
    }
}

char *md5(void *buf, size_t len) {
    md5_byte_t md5[16];
    md5_state_t md5_state;

    md5_init(&md5_state);
    md5_append(&md5_state, buf, len);
    md5_finish(&md5_state, md5);

    char *md5_hex = malloc(33);
    md5_hex[32] = '\0';
    int i;
    for (i = 0; i < 16; i++) {
        snprintf(&(md5_hex[i*2]), 3, "%02x", md5[i]);
    }
    log_debug("md5 hex: %s", md5_hex);
    return md5_hex;
}
