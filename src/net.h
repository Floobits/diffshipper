#ifndef NET_H
#define NET_H

#include <pthread.h>

#define DS_PROTO_VERSION "0.01"

struct addrinfo *server_info;
int server_sock;
pthread_cond_t server_conn_ready;
pthread_mutex_t server_conn_mtx;

void *net_buf;
ssize_t net_buf_len;
ssize_t net_buf_size;

int server_connect(const char *host, const char *port);

ssize_t send_json(const char *fmt, ...);

void *remote_change_worker();

void net_cleanup();

#endif
