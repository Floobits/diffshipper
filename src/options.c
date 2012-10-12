#include <getopt.h>
#include <string.h>

#include "log.h"
#include "options.h"

void init_opts() {
    memset(&opts, 0, sizeof(opts));
}

void parse_opts(int argc, char **argv) {
    char ch;
    int opt_index = 0;

    struct option longopts[] = {
        {"host", required_argument, NULL, 'h'},
        {"port", required_argument, NULL, 'p'}
    };

    while ((ch = getopt_long(argc, argv, "h:p:", longopts, &opt_index)) != -1) {
        switch (ch) {
            case 'h':
                opts.host = optarg;
            break;
            case 'p':
                opts.port = optarg;
            break;
            default:
                die("I don't understand you!");
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 1)
        die("No path to watch specified");

    opts.path = realpath(argv[0], NULL);

    if (!opts.host) {
        asprintf(&opts.host, "127.0.0.1");
    }
    if (!opts.port) {
        asprintf(&opts.port, "3148");
    }

    log_debug("options: host %s port %s path %s", opts.host, opts.port, opts.path);
}
