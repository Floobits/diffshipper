#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log.h"
#include "options.h"
#include "util.h"


void print_version() {
    printf("diffshipper version %s\n", PACKAGE_VERSION);
}


void usage() {
    printf("Usage: diffshipper [OPTIONS] PATH\n\
\n\
-h HOST \n\
-p PORT \n\
\n");
}


void init_opts() {
    memset(&opts, 0, sizeof(opts));
    opts.mtime = 2; /* seconds */
}


void parse_opts(int argc, char **argv) {
    char ch;
    int opt_index = 0;

    struct option longopts[] = {
        {"debug", no_argument, NULL, 'D'},
        {"host", required_argument, NULL, 'h'},
        {"owner", required_argument, NULL, 'o'},
        {"port", required_argument, NULL, 'p'},
        {"room", required_argument, NULL, 'r'},
        {"secret", required_argument, NULL, 's'},
        {"username", required_argument, NULL, 'u'},
        {"version", no_argument, NULL, 'v'},
        { NULL, 0, NULL, 0 }
    };

    while ((ch = getopt_long(argc, argv, "Dh:o:p:r:s:u:v", longopts, &opt_index)) != -1) {
        switch (ch) {
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
                exit(1);
            break;
            default:
                usage();
                exit(1);
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 1)
        die("No path to watch specified");

    opts.path = realpath(argv[0], NULL);

    if (!opts.host) {
        ds_asprintf(&opts.host, "127.0.0.1");
    }
    if (!opts.port) {
        ds_asprintf(&opts.port, "3148");
    }

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
