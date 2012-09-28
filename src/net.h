#ifndef NET_H
#define NET_H

struct addrinfo *server_info;
int server_sock;

int server_connect(const char *host, const char *port);
int send_bytes(const void *buf, const size_t len);
void net_cleanup();

#endif
