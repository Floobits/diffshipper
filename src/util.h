#ifndef UTIL_H
#define UTIL_H

#include <dirent.h>

#include <jansson.h>


typedef long long int lli_t;

int run_cmd(const char *fmt, ...);

int binary_search(const char* needle, char **haystack, int start, int end);

char *escape_data(const char *data, int len);
char *unescape_data(char *escaped);

void ds_asprintf(char **ret, const char *fmt, ...);

int is_binary(const void* buf, const int buf_len);
int is_directory(const char *path, const struct dirent *d);
int is_symlink(const char *path, const struct dirent *d);

void parse_json(json_t *json_obj, const char *fmt, ...);

char *md5(void *buf, size_t len);

#endif
