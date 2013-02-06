#ifndef IGNORE_H
#define IGNORE_H

#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>

#define SVN_DIR_PROP_BASE "dir-prop-base"
#define SVN_DIR ".svn"
#define SVN_PROP_IGNORE "svn:ignore"

typedef struct ignores_t {
    char **names; /* Non-regex ignore lines. Sorted so we can binary search them. */
    size_t names_len;
    char **regexes; /* For patterns that need fnmatch */
    size_t regexes_len;
    struct ignores_t *parent;
} ignores_t;

ignores_t *root_ignores;

extern const char *evil_hardcoded_ignore_files[];
extern const char *ignore_pattern_files[];

typedef struct {
	char *path;
	time_t changed_at;
} ignored_change_t;


char **ignored_changes;
int ignored_changes_len;
pthread_mutex_t ignore_changes_mtx;

void ignore_change(const char *path);
void unignore_change(const char *path);
int is_ignored(const char *path);

ignores_t *init_ignore(ignores_t *parent);
void cleanup_ignore(ignores_t *ig);

void add_ignore_pattern(ignores_t *ig, const char* pattern);

void load_ignore_patterns(ignores_t *ig, const char *path);
void load_svn_ignore_patterns(ignores_t *ig, const char *path);

int scandir_filter(const char *path, const struct dirent *dir, void *baton);

#endif
