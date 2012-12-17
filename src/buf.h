#ifndef BUF_H
#define BUF_H

typedef struct {
    int id;
    char *buf;
    char *md5;
    char *path;
} buf_t;

void save_buf(buf_t *buf);
void apply_patch(buf_t *buf, char *patch_text);

#endif
