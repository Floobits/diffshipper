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

char *get_full_path(char *rel_path);

buf_t *get_buf_by_id(const int buf_id);
buf_t *get_buf(const char *path);

void add_buf_to_bufs(buf_t *buf);
void delete_buf(buf_t *buf);
void save_buf(buf_t *buf);

int apply_patch(buf_t *buf, char *patch_text);

#endif
