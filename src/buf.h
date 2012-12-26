#ifndef BUF_H
#define BUF_H

typedef struct {
    int id;
    char *buf;
    size_t len;
    char *md5;
    char *path;
} buf_t;

buf_t **bufs;
size_t bufs_len;
size_t bufs_size;
#define BUFS_START_SIZE 100

void init_bufs();
void cleanup_bufs();

buf_t *get_buf(const char *path);
void save_buf(buf_t *buf);

void apply_patch(buf_t *buf, char *patch_text);

#endif
