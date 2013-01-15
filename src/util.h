#ifndef UTIL_H
#define UTIL_H

#include <pthread.h>

#include <jansson.h>


typedef long long int lli_t;

char **ignored_paths;
int ignored_paths_len;
pthread_mutex_t ignore_mtx;

int run_cmd(const char *fmt, ...);

void ignore_path(const char *path);
void unignore_path(const char *path);
int ignored(const char *path);

char *escape_data(const char *data, int len);
char *unescape_data(char *escaped);

void ds_asprintf(char **ret, const char *fmt, ...);

int is_binary(const void* buf, const int buf_len);

void parse_json(json_t *json_obj, const char *fmt, ...);

char *md5(void *buf, size_t len);

#endif
