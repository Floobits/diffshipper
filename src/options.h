#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct {
    char *host;
    char *port;
    char *path;
} options_t;

options_t opts;

void init_opts();

void parse_opts(int argc, char **argv);

#endif
