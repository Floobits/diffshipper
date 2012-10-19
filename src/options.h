#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct {
    char *host;
    char *port;
    char *path;
    int mtime;
    char *username;
    char *secret;
    char *room;
} options_t;

options_t opts;

void print_version();
void usage();

void init_opts();
void parse_opts(int argc, char **argv);


#endif
