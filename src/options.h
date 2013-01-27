#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct {
    char *api_url;
    char *host;
    char *port;
    char *path;
    int mtime;
    char *username;
    char *secret;
    char *owner;
    char *room;
    int create_room;
    int delete_room;
    int room_perms;
    int read_only;
} options_t;

options_t opts;

void init_opts();
void parse_opts(int argc, char **argv);

#endif
