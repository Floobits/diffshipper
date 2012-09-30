#ifndef NET_H
#define NET_H

struct addrinfo *server_info;
int server_sock;

#define NET_HEADER_LEN 20

int server_connect(const char *host, const char *port);

ssize_t send_bytes(const void *buf, const size_t len);
ssize_t recv_bytes(void **buf, size_t len);

void net_cleanup();

#endif
