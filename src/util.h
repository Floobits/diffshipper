#ifndef UTIL_H
#define UTIL_H

#include <pthread.h>

#include <jansson.h>

#ifdef INOTIFY
#include <inotifytools/inotifytools.h>
#include <inotifytools/inotify.h>
#endif

#ifdef FSEVENTS
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef long long int lli_t;

char **ignored_paths;
int ignored_paths_len;
pthread_mutex_t ignore_mtx;

int run_cmd(const char *fmt, ...);

void ignore_path(const char *path);
void unignore_path(const char *path);
int ignored(const char *path);

char *escape_data(char *data);
char *unescape_data(char *data);

void ds_asprintf(char **ret, const char *fmt, ...);

int is_binary(const void* buf, const int buf_len);

void parse_json(json_t *json_obj, const char *fmt, ...);

#endif
