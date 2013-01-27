#ifndef IGNORE_H
#define IGNORE_H

#include <pthread.h>


char **ignored_changes;
int ignored_changes_len;
pthread_mutex_t ignore_changes_mtx;

void ignore_change(const char *path);
void unignore_change(const char *path);
int is_ignored(const char *path);

#endif
