#include <errno.h>
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
    int rv;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    rv = getaddrinfo(host, port, &hints, &server_info);
    if (rv != 0)
        die("getaddrinfo() error: %s", strerror(errno));

    server_sock = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (server_sock == -1)
        die("socket() error: %s", strerror(errno));

    log_debug("Connecting to %s:%s...", host, port);
    rv = connect(server_sock, server_info->ai_addr, server_info->ai_addrlen);
    if (rv == -1)
        die("connect() error: %s", strerror(errno));

    log_debug("Connected to %s:%s", host, port);

    char msg[] = "hello!";
    ssize_t bytes_sent = send(server_sock, &msg, strlen(msg), 0);
    if (bytes_sent == -1)
        die("send() error: %s", strerror(errno));
    /* TODO: check # of bytes sent was correct */

    return rv;
}


int send_bytes(const void *buf, const size_t len) {
    int bytes_sent = send(server_sock, buf, len, 0);
    if (bytes_sent == -1)
        die("send() error: %s", strerror(errno));
    return bytes_sent;
}

ssize_t recv_bytes(void *buf, size_t len) {
    ssize_t bytes_received;
    /* TODO: check if bytes received is 0 and reconnect */
    bytes_received = recv(server_sock, buf, len, 0);
    log_debug("received %u bytes", len);
    fwrite(buf, (size_t)len, 1, stdout);
    return bytes_received;
}

void net_cleanup() {
    close(server_sock);
    freeaddrinfo(server_info);
}
