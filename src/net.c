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
    ssize_t bytes_sent = send_bytes(&msg, strlen(msg));
    if (bytes_sent == -1)
        die("send_bytes() error: %s", strerror(errno));
    /* TODO: check # of bytes sent was correct */

    return rv;
}


ssize_t send_bytes(const void *buf, const size_t len) {
    if (len == 0) {
        log_err("nothing to send. wtf?");
        return 0;
    }
    int total_len = NET_HEADER_LEN + len;
    char *net_buf = malloc((total_len) * sizeof(char));
    snprintf(net_buf, NET_HEADER_LEN + 1, "%020lu", len);
    memcpy(net_buf + NET_HEADER_LEN + 1, buf, len);
    ssize_t bytes_sent = send(server_sock, net_buf, total_len, 0);
    free(net_buf);
    if (bytes_sent == -1)
        die("send() error: %s", strerror(errno));
    return bytes_sent;
}


ssize_t recv_bytes(void **buf) {
    ssize_t bytes_received;
    *buf = realloc(*buf, NET_HEADER_LEN);
    /* TODO: check if bytes received is 0 and reconnect */
    bytes_received = recv(server_sock, *buf, NET_HEADER_LEN, 0);
    log_debug("received %u bytes", bytes_received);
    fwrite(*buf, (size_t)bytes_received, 1, stdout);
    if (bytes_received != NET_HEADER_LEN) {
        die("fix this code so it retries or something");
    }
    char header[NET_HEADER_LEN + 1];
    memcpy(header, *buf, NET_HEADER_LEN);
    header[NET_HEADER_LEN] = '\0';
    int msg_len = atoi(header);
    log_debug("msg is %i bytes", msg_len);
    /* TODO: check to make sure we got the whole message */
    *buf = realloc(*buf, msg_len);
    bytes_received = recv(server_sock, *buf, msg_len, 0);
    return bytes_received;
}


void net_cleanup() {
    close(server_sock);
    freeaddrinfo(server_info);
}
