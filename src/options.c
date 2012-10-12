#include <string.h>

#include "log.h"
#include "options.h"

void init_opts() {
    memset(&opts, 0, sizeof(opts));
}

void parse_opts(int argc, char **argv) {
    if (argc < 2)
        die("No path to watch specified");
    opts.path = realpath(argv[1], NULL);

}
