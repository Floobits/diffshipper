#ifndef UTIL_H
#define UTIL_H

#include <pthread.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

char **ignored_paths;
int ignored_paths_len;
pthread_mutex_t ignore_mtx;

int run_cmd(const char *fmt, ...);

void ignore_path(const char *path);
void unignore_path(const char *path);
int ignored(const char *path);

char *escape_data(char *data);
char *unescape_data(char *data);

#endif
