#include <ctype.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ignore.h"
#include "log.h"
#include "scandir.h"
#include "util.h"


const char *evil_hardcoded_ignore_files[] = {
    ".",
    "..",
    NULL
};

/* Warning: changing the first string will break skip_vcs_ignores_t. */
const char *ignore_pattern_files[] = {
    ".dsignore",
    ".gitignore",
    ".git/info/exclude",
    ".hgignore",
    ".svn",
    NULL
};

const int fnmatch_flags = 0 & FNM_PATHNAME;


void ignore_change(const char *path) {
    int i;

    pthread_mutex_lock(&ignore_changes_mtx);
    ignored_changes_len++;
    ignored_changes = realloc(ignored_changes, ignored_changes_len * sizeof(char*));
    /* TODO: do a binary search to figure out the position */
    for (i = ignored_changes_len - 1; i > 0; i--) {
        if (strcmp(path, ignored_changes[i-1]) > 0) {
            break;
        }
        ignored_changes[i] = ignored_changes[i-1];
    }
    ignored_changes[i] = strdup(path);
    log_debug("ignoring path %s", path);
    pthread_mutex_unlock(&ignore_changes_mtx);
}


void unignore_change(const char *path) {
    int i;
    /* TODO: do a binary search to figure out the position */
    pthread_mutex_lock(&ignore_changes_mtx);
    /* totally unsafe */
    for (i = 0; i < ignored_changes_len - 1; i++) {
        if (strcmp(path, ignored_changes[i]) >= 0) {
            ignored_changes[i] = ignored_changes[i+1];
        }
    }
    ignored_changes_len--;
    ignored_changes = realloc(ignored_changes, ignored_changes_len * sizeof(char*));
    log_debug("unignoring path %s", path);
    pthread_mutex_unlock(&ignore_changes_mtx);
}


int is_ignored(const char *path) {
    int i;
    int rv = 0;
    pthread_mutex_lock(&ignore_changes_mtx);
    /* TODO: binary search */
    for (i = 0; i < ignored_changes_len; i++) {
        if (strcmp(ignored_changes[i], path) == 0) {
            rv = 1;
            break;
        }
    }
    pthread_mutex_unlock(&ignore_changes_mtx);
    if (rv) {
        log_debug("file %s is ignored", path);
    } else {
        log_debug("file %s is not ignored", path);
    }
    return rv;
}


ignores_t *init_ignore(ignores_t *parent) {
    ignores_t *ig = malloc(sizeof(ignores_t));
    ig->names = NULL;
    ig->names_len = 0;
    ig->regexes = NULL;
    ig->regexes_len = 0;
    ig->parent = parent;
    return ig;
}


void cleanup_ignore(ignores_t *ig) {
    size_t i;

    if (ig) {
        if (ig->regexes) {
            for (i = 0; i < ig->regexes_len; i++) {
                free(ig->regexes[i]);
            }
            free(ig->regexes);
        }
        if (ig->names) {
            for (i = 0; i < ig->names_len; i++) {
                free(ig->names[i]);
            }
            free(ig->names);
        }
        free(ig);
    }
}


static int is_fnmatch(const char* filename) {
    char fnmatch_chars[] = {
        '!',
        '*',
        '?',
        '[',
        ']',
        '\0'
    };

    return (strpbrk(filename, fnmatch_chars) != NULL);
}


void add_ignore_pattern(ignores_t *ig, const char* pattern) {
    int i;
    int pattern_len;

    /* Strip off the leading ./ so that matches are more likely. */
    if (strncmp(pattern, "./", 2) == 0) {
        pattern += 2;
    }

    /* Kill trailing whitespace */
    for (pattern_len = strlen(pattern); pattern_len > 0; pattern_len--) {
        if (!isspace(pattern[pattern_len-1])) {
            break;
        }
    }

    if (pattern_len == 0) {
        log_debug("Pattern is empty. Not adding any ignores_t.");
        return;
    }

    /* TODO: de-dupe these patterns */
    if (is_fnmatch(pattern)) {
        ig->regexes_len++;
        ig->regexes = realloc(ig->regexes, ig->regexes_len * sizeof(char*));
        ig->regexes[ig->regexes_len - 1] = strndup(pattern, pattern_len);
        log_debug("added regex ignore pattern %s", pattern);
    } else {
        /* a balanced binary tree is best for performance, but I'm lazy */
        ig->names_len++;
        ig->names = realloc(ig->names, ig->names_len * sizeof(char*));
        for (i = ig->names_len - 1; i > 0; i--) {
            if (strcmp(pattern, ig->names[i-1]) > 0) {
                break;
            }
            ig->names[i] = ig->names[i-1];
        }
        ig->names[i] = strndup(pattern, pattern_len);
        log_debug("added literal ignore pattern %s", pattern);
    }
}


/* For loading git/svn/hg ignore patterns */
void load_ignore_patterns(ignores_t *ig, const char *path) {
    FILE *fp = NULL;
    fp = fopen(path, "r");
    if (fp == NULL) {
        log_debug("Skipping ignore file %s", path);
        return;
    }

    char *line = NULL;
    ssize_t line_len = 0;
    size_t line_cap = 0;

    while ((line_len = getline(&line, &line_cap, fp)) > 0) {
        if (line_len == 0 || line[0] == '\n' || line[0] == '#') {
            continue;
        }
        if (line[line_len-1] == '\n') {
            line[line_len-1] = '\0'; /* kill the \n */
        }
        add_ignore_pattern(ig, line);
    }

    free(line);
    fclose(fp);
}


void load_svn_ignore_patterns(ignores_t *ig, const char *path) {
    FILE *fp = NULL;
    char *dir_prop_base;
    ds_asprintf(&dir_prop_base, "%s/%s", path, SVN_DIR_PROP_BASE);

    fp = fopen(dir_prop_base, "r");
    if (fp == NULL) {
        log_debug("Skipping svn ignore file %s", dir_prop_base);
        free(dir_prop_base);
        return;
    }

    char *entry = NULL;
    size_t entry_len = 0;
    char *key = malloc(32); /* Sane start for max key length. */
    size_t key_len = 0;
    size_t bytes_read = 0;
    char *entry_line;
    size_t line_len;
    int matches;

    while (fscanf(fp, "K %zu\n", &key_len) == 1) {
        key = realloc(key, key_len + 1);
        bytes_read = fread(key, 1, key_len, fp);
        key[key_len] = '\0';
        matches = fscanf(fp, "\nV %zu\n", &entry_len);
        if (matches != 1) {
            log_debug("Unable to parse svnignore file %s: fscanf() got %i matches, expected 1.", dir_prop_base, matches);
            goto cleanup;
        }

        if (strncmp(SVN_PROP_IGNORE, key, bytes_read) != 0) {
            log_debug("key is %s, not %s. skipping %u bytes", key, SVN_PROP_IGNORE, entry_len);
            /* Not the key we care about. fseek and repeat */
            fseek(fp, entry_len + 1, SEEK_CUR); /* +1 to account for newline. yes I know this is hacky */
            continue;
        }
        /* Aww yeah. Time to ignore stuff */
        entry = malloc(entry_len + 1);
        bytes_read = fread(entry, 1, entry_len, fp);
        entry[bytes_read] = '\0';
        log_debug("entry: %s", entry);
        break;
    }
    if (entry == NULL) {
        goto cleanup;
    }
    char *patterns = entry;
    while (*patterns != '\0' && patterns < (entry + bytes_read)) {
        for (line_len = 0; line_len < strlen(patterns); line_len++) {
            if (patterns[line_len] == '\n') {
                break;
            }
        }
        if (line_len > 0) {
            entry_line = strndup(patterns, line_len);
            add_ignore_pattern(ig, entry_line);
            free(entry_line);
        }
        patterns += line_len + 1;
    }
    free(entry);
    cleanup:;
    free(dir_prop_base);
    free(key);
    fclose(fp);
}


static int filename_ignore_search(const ignores_t *ig, const char *filename) {
    size_t i;
    int match_pos;

    if (strncmp(filename, "./", 2) == 0) {
        filename += 2;
    }

    match_pos = binary_search(filename, ig->names, 0, ig->names_len);
    if (match_pos >= 0) {
        log_debug("file %s ignored because name matches static pattern %s", filename, ig->names[match_pos]);
        return 1;
    }

    for (i = 0; i < ig->regexes_len; i++) {
        if (fnmatch(ig->regexes[i], filename, fnmatch_flags) == 0) {
            log_debug("file %s ignored because name matches regex pattern %s", filename, ig->regexes[i]);
            return 1;
        }
    }
    return 0;
}


static int path_ignore_search(const ignores_t *ig, const char *path, const char *filename) {
    char *temp;

    if (filename_ignore_search(ig, filename)) {
        return 1;
    }
    ds_asprintf(&temp, "%s/%s", path, filename);
    int rv = filename_ignore_search(ig, temp);
    free(temp);
    return rv;
}


/* This function is REALLY HOT. It gets called for every file */
int scandir_filter(const char *path, const struct dirent *dir, void *baton) {
    const char *filename = dir->d_name;
    size_t i;
    scandir_baton_t *scandir_baton = (scandir_baton_t*) baton;
    const ignores_t *ig = scandir_baton->ig;
    const char *base_path = scandir_baton->base_path;
    const char *path_start = path;
    char *temp;

    if (is_symlink(path, dir)) {
        log_debug("File %s ignored becaused it's a symlink", dir->d_name);
        return 0;
    }

    for (i = 0; evil_hardcoded_ignore_files[i] != NULL; i++) {
        if (strcmp(filename, evil_hardcoded_ignore_files[i]) == 0) {
            return 0;
        }
    }

    if (filename[0] == '.') {
        log_debug("File %s ignored because it's hidden", dir->d_name);
        return 0;
    }

    for (i = 0; base_path[i] == path[i] && i < strlen(base_path); i++) {
        /* base_path always ends with "/\0" while path doesn't, so this is safe */
        path_start = path + i + 2;
    }
    log_debug("path_start is %s", path_start);

    if (path_ignore_search(ig, path_start, filename)) {
        return 0;
    }

    if (is_directory(path, dir) && filename[strlen(filename) - 1] != '/') {
        ds_asprintf(&temp, "%s/", filename);
        int rv = path_ignore_search(ig, path_start, temp);
        free(temp);
        if (rv) {
            return 0;
        }
    }

    /* TODO: copy-pasted from above */
    if (scandir_baton->level == 0) {
        /* Obey .gitignore patterns that start with a / */
        char *temp2; /* horrible variable name, I know */
        ds_asprintf(&temp, "/%s", filename);
        if (path_ignore_search(ig, path_start, temp)) {
            return 0;
        }

        if (is_directory(path, dir) && temp[strlen(filename) - 1] != '/') {
            ds_asprintf(&temp2, "%s/", temp);
            int rv = path_ignore_search(ig, path_start, temp2);
            free(temp2);
            if (rv) {
                return 0;
            }
        }
        free(temp);
    }

    scandir_baton->level++;
    if (ig->parent != NULL) {
        scandir_baton->ig = ig->parent;
        return scandir_filter(path, dir, (void *)scandir_baton);
    }

    return 1;
}
