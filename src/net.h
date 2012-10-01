#ifndef NET_H
#define NET_H

#include <pthread.h>

struct addrinfo *server_info;
int server_sock;
pthread_cond_t server_conn_ready;

void *net_buf;
ssize_t net_buf_len;

int server_connect(const char *host, const char *port);

ssize_t send_bytes(const void *buf, const size_t len);
ssize_t recv_bytes(void **buf);

void *remote_change_watcher();

void net_cleanup();

#endif
