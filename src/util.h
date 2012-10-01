#ifndef UTIL_H
#define UTIL_H

#include <pthread.h>

char **ignored_paths;
int ignored_paths_len;
pthread_mutex_t ignore_mtx;

int run_cmd(const char *fmt, ...);

void ignore_path(const char *path);
void unignore_path(const char *path);
int ignored(const char *path);

#endif
