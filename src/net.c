#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "log.h"
#include "net.h"
#include "util.h"

int server_connect(const char *host, const char *port) {
    int rv;
    rv = getaddrinfo(host, port, NULL, &server_info);
    if (rv != 0)
        die("error connecting to server: %s", strerror(errno));
    

    return rv;
}

void net_cleanup() {
    freeaddrinfo(server_info);
}