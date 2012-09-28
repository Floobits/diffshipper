#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "net.h"
#include "util.h"


int server_connect(const char *host, const char *port) {
    int flags;
    int rv;
    struct addrinfo hints;
    fd_set read_fds;
    struct timeval tv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    rv = getaddrinfo(host, port, &hints, &server_info);
    if (rv != 0)
        die("getaddrinfo() error: %s", strerror(errno));

    server_sock = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (server_sock == -1)
        die("socket() error: %s", strerror(errno));

    flags = fcntl(server_sock, F_GETFL, 0);
    rv = fcntl(server_sock, F_SETFL, flags | O_NONBLOCK);
    if (rv == -1)
        die("fcntl() error: %s", strerror(errno));

    log_debug("Connecting to %s:%s...", host, port);
    rv = connect(server_sock, server_info->ai_addr, server_info->ai_addrlen);
    if (rv != -1 || errno != EINPROGRESS)
        die("connect() error: %s", strerror(errno));

    FD_ZERO(&read_fds);
    FD_SET(server_sock, &read_fds);
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    rv = select(1, &read_fds, NULL, NULL, &tv);
    if (rv == -1) {
        die("select() error:", strerror(errno));
    } else if (rv == 0) {
        /* timeout */
        die("Timed out connecting to %s:%s.", host, port);
    }

    log_debug("Connected to %s:%s", host, port);

    char msg[] = "hello!";
    ssize_t bytes_sent = send(server_sock, &msg, strlen(msg), 0);
    if (bytes_sent == -1)
        die("send() error: %s", strerror(errno));

    return rv;
}


void net_cleanup() {
    close(server_sock);
    freeaddrinfo(server_info);
}
