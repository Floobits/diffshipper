#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log.h"
#include "options.h"
#include "util.h"


static void print_version() {
    printf("Floobits Diff Shipper v%s\n", PACKAGE_VERSION);
}


static void usage() {
    printf("Usage: diffshipper -r ROOM_NAME -o ROOM_OWNER -u USERNAME -s API_SECRET [OPTIONS] PATH\n\
\n\
    --api-url URL       For debugging/development. Defaults to https://floobits.com/api/room/\n\
    --create-room       Create a room and add PATH\n\
    --delete-room       Delete a room. Can be used with --create-room.\n\
    -D                  Enable debug output\n\
    -h HOST             For debugging/development. Defaults to floobits.com.\n\
    -o OWNER            Room owner\n\
    -p PORT             For debugging/development. Defaults to 3148.\n\
    -r ROOMNAME         Room to join\n\
    --recreate-room     Delete the room if it exists, then create it again\n\
    --room-perms PERM   Used with --[re]create-room. 0 = private, 1 = readable by anyone, 2 = writeable by anyone\n\
    -s SECRET           API secret\n\
    -u USERNAME         Username\n\
    -v                  Print version and exit\n\
\n");
    exit(1);
}


void init_opts() {
    memset(&opts, 0, sizeof(opts));
    opts.mtime = 2; /* seconds */
    opts.room_perms = -1;
}


void parse_opts(int argc, char **argv) {
    char ch;
    const char *long_opt;
    int opt_index = 0;

    struct option longopts[] = {
        {"api-url", required_argument, NULL, 0},
        {"create-room", no_argument, NULL, 'c'},
        {"delete-room", no_argument, NULL, 0},
        {"debug", no_argument, NULL, 'D'},
        {"help", no_argument, NULL, 0},
        {"host", required_argument, NULL, 'h'},
        {"owner", required_argument, NULL, 'o'},
        {"port", required_argument, NULL, 'p'},
        {"recreate-room", no_argument, NULL, 0},
        {"read-only", no_argument, NULL, 0},
        {"room", required_argument, NULL, 'r'},
        {"room-perms", required_argument, NULL, 0},
        {"secret", required_argument, NULL, 's'},
        {"username", required_argument, NULL, 'u'},
        {"version", no_argument, NULL, 'v'},
        { NULL, 0, NULL, 0 }
    };

    while ((ch = getopt_long(argc, argv, "Dh:o:p:r:s:u:v", longopts, &opt_index)) != -1) {
        switch (ch) {
            case 'c':
                opts.create_room = 1;
            break;
            case 'D':
                set_log_level(LOG_LEVEL_DEBUG);
            case 'h':
                opts.host = optarg;
            break;
            case 'o':
                opts.owner = optarg;
            break;
            case 'p':
                opts.port = optarg;
            break;
            case 'r':
                opts.room = optarg;
            break;
            case 's':
                opts.secret = optarg;
            break;
            case 'u':
                opts.username = optarg;
            break;
            case 'v':
                print_version();
                exit(0);
            break;
            case 0: /* Long option */
                long_opt = longopts[opt_index].name;
                if (strcmp(long_opt, "api-url") == 0) {
                    opts.api_url = optarg;
                } else if (strcmp(long_opt, "delete-room") == 0) {
                    opts.delete_room = 1;
                } else if (strcmp(long_opt, "help") == 0) {
                    usage();
                } else if (strcmp(long_opt, "recreate-room") == 0) {
                    opts.create_room = 1;
                    opts.delete_room = 1;
                } else if (strcmp(long_opt, "read-only") == 0) {
                    opts.read_only = 1;
                } else if (strcmp(long_opt, "room-perms") == 0) {
                    opts.room_perms = atoi(optarg);
                } else {
                    printf("Unrecognized option: %s\n\n", long_opt);
                    usage();
                }
            break;
            default:
                printf("Unrecognized option: %c\n\n", ch);
                usage();
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 1)
        usage();

    opts.path = realpath(argv[0], NULL);

    if (!opts.host)
        ds_asprintf(&opts.host, "floobits.com");

    if (!opts.api_url)
        ds_asprintf(&opts.api_url, "https://%s/api/room/", opts.host);

    if (!opts.port)
        ds_asprintf(&opts.port, "3148");

    if (!opts.owner)
        die("No room owner specified");

    if (!opts.room)
        die("No room specified");

    if (!opts.secret)
        die("No secret specified");

    if (!opts.username)
        die("No username specified");

    log_debug("options: host %s port %s path %s", opts.host, opts.port, opts.path);
}
