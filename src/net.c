#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "diff.h"
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

    char msg[] = "{\"v\": \"0.01\"}\n";
    ssize_t bytes_sent = send_bytes(&msg, strlen(msg));
    if (bytes_sent == -1)
        die("send_bytes() error: %s", strerror(errno));
    /* TODO: check # of bytes sent was correct */

    net_buf = malloc(100);
    net_buf_len = 0;
    net_buf_size = 100;
    pthread_cond_broadcast(&server_conn_ready);

    return rv;
}


ssize_t send_bytes(const void *buf, const size_t len) {
    if (len == 0) {
        log_err("nothing to send. wtf?");
        return 0;
    }
    ssize_t bytes_sent = send(server_sock, buf, len, 0);
    if (bytes_sent == -1)
        die("send() error: %s", strerror(errno));
    return bytes_sent;
}


ssize_t recv_bytes(char **buf) {
    ssize_t bytes_received;
    ssize_t buf_len;
    void *net_buf_end;
    ssize_t net_buf_left;
    void *line_end;

    /* TODO: check if bytes received is 0 and reconnect */
    do {
        net_buf_end = net_buf + net_buf_len;
        net_buf_left = net_buf_size - net_buf_len;
        bytes_received = recv(server_sock, net_buf_end, net_buf_left, 0);
        net_buf_len += bytes_received;
        log_debug("received %u bytes", bytes_received);
        if (bytes_received == 0) {
            die("disconnected from server");
        } else if (bytes_received == net_buf_left) {
            net_buf_size *= 1.5;
            net_buf = realloc(net_buf, net_buf_size);
        }
        line_end = memchr(net_buf, '\n', net_buf_len);
    } while(line_end == NULL);

    buf_len = line_end - net_buf;
    *buf = realloc(*buf, buf_len + 1);
    memcpy(*buf, net_buf, buf_len);
    buf[buf_len] = '\0';
    memmove(net_buf, line_end + 1, buf_len - 1);
    net_buf_len -= buf_len + 1;
    return buf_len;
}


void *remote_change_worker() {
    char *buf = NULL;
    ssize_t rv;
    char *path;
    char action;
    char *diff_data;
    unsigned int diff_pos;
    unsigned int diff_size;
    dmp_operation_t op;

    pthread_cond_wait(&server_conn_ready, &server_conn_mtx);
    pthread_mutex_unlock(&server_conn_mtx);

    while (TRUE) {
        rv = recv_bytes(&buf);
        if (!rv) {
            /* TODO: reconnect or error out or something*/
            log_err("no bytes!");
            continue;
        }
        /* yeah this is retarded */
        path = malloc(1000);
        diff_data = malloc(100000);
        rv = sscanf(buf, "{ \"path\": \"%[^\"]\", \"action\": \"%c%u@%u\", \"data\": \"%[^\"] }\n", path, &action, &diff_size, &diff_pos, diff_data);
        if (rv != 5) {
            log_warn("rv %i. unable to parse message: %s", rv, buf);
            log_debug("path %s action %c diff_size %u diff_pos %u data %s", path, action, diff_size, diff_pos, diff_data);
            goto cleanup;
        }
        log_debug("parsed { \"path\": \"%s\", \"action\": \"%c%u@%u\", \"data\": \"%s\" }\n", path, action, diff_size, diff_pos, diff_data);
        ignore_path(path);
        if (action == '+') {
            op = DMP_DIFF_INSERT;
        } else if (action == '-') {
            op = DMP_DIFF_DELETE;
        } else {
            die("unknown action: %c", action);
        }
        apply_diff(path, op, diff_data, diff_size, diff_pos);

        cleanup:;
        free(path);
        free(diff_data);
    }

    free(buf);
    pthread_exit(NULL);
    return NULL;
}


void net_cleanup() {
    close(server_sock);
    freeaddrinfo(server_info);
}
