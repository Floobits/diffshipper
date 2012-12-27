#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <jansson.h>

#include "log.h"
#include "net.h"
#include "options.h"


int server_connect(const char *host, const char *port) {
    int rv;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    rv = getaddrinfo(host, port, &hints, &server_info);
    if (rv)
        die("getaddrinfo() error: %s", strerror(errno));

    server_sock = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (server_sock == -1)
        die("socket() error: %s", strerror(errno));

    log_debug("Connecting to %s:%s...", host, port);
    rv = connect(server_sock, server_info->ai_addr, server_info->ai_addrlen);
    if (rv == -1)
        die("connect() error: %s", strerror(errno));

    log_debug("Connected to %s:%s", host, port);
    send_json("{s:s s:s s:s s:s s:s}",
        "version", DS_PROTO_VERSION,
        "username", opts.username,
        "secret", opts.secret,
        "room_owner", opts.owner,
        "room", opts.room
    );

    net_buf = malloc(100);
    net_buf_len = 0;
    net_buf_size = 100;
    pthread_cond_broadcast(&server_conn_ready);

    return rv;
}


static ssize_t send_bytes(const void *buf, const size_t len) {
    if (len == 0) {
        log_err("nothing to send. wtf?");
        return 0;
    }
    ssize_t bytes_sent = send(server_sock, buf, len, 0);
    if (bytes_sent == -1)
        die("send() error: %s", strerror(errno));
    return bytes_sent;
}


static ssize_t recv_bytes(char **buf) {
    ssize_t bytes_received;
    ssize_t buf_len;
    void *net_buf_end;
    ssize_t net_buf_left;
    void *line_end;

    do {
        net_buf_end = net_buf + net_buf_len;
        net_buf_left = net_buf_size - net_buf_len;
        bytes_received = recv(server_sock, net_buf_end, net_buf_left, 0);
        net_buf_len += bytes_received;
        log_debug("received %u bytes", bytes_received);
        if (bytes_received == 0) {
            return 0;
        } else if (bytes_received == net_buf_left) {
            net_buf_size *= 1.5;
            net_buf = realloc(net_buf, net_buf_size);
        }
        line_end = memchr(net_buf, '\n', net_buf_len);
    } while(line_end == NULL);

    buf_len = line_end - net_buf;
    *buf = realloc(*buf, buf_len + 1);
    memcpy(*buf, net_buf, buf_len);
    (*buf)[buf_len] = '\0';
    memmove(net_buf, line_end + 1, buf_len - 1);
    net_buf_len -= buf_len + 1;
    return buf_len;
}


json_t *recv_json() {
    char *buf = NULL;
    ssize_t rv;
    json_t *json_obj;
    json_error_t json_err;

    rv = recv_bytes(&buf);
    if (!rv) {
        /* TODO: reconnect or error out or something*/
        die("no bytes!");
    }

    json_obj = json_loadb(buf, rv, 0, &json_err);
    if (!json_obj) {
        log_json_err(&json_err);
        die("couldn't load buffer into json object!");
    }

    free(buf);

    return json_obj;
}


ssize_t send_json(const char *fmt, ...) {
    char *msg;
    size_t msg_len;
    json_error_t json_err;
    va_list args;
    json_t *json_obj;

    va_start(args, fmt);
    json_obj = json_vpack_ex(&json_err, 0, fmt, args);
    va_end(args);

    if (!json_obj) {
        log_json_err(&json_err);
        die("error packing json");
    }

    msg = json_dumps(json_obj, JSON_ENSURE_ASCII);
    if (!msg) {
        die("error dumping json");
        return -1;
    }
    msg_len = strlen(msg) + 1;
    msg = realloc(msg, msg_len+1);
    strcat(msg, "\n");

    log_debug("sending json: %s", msg);

    ssize_t bytes_sent = send_bytes(msg, msg_len);
    if (bytes_sent != (ssize_t)msg_len)
        die("tried to send %u bytes but only sent %i", msg_len, bytes_sent);

    free(msg);
    json_decref(json_obj);
    return bytes_sent;
}


void net_cleanup() {
    close(server_sock);
    freeaddrinfo(server_info);
}
