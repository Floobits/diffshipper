#ifndef BUF_H
#define BUF_H

typedef struct {
    int id;
    char *buf;
    char *md5;
    char *path;
} buf_t;

int set_buf(buf_t *buf);

#endif
