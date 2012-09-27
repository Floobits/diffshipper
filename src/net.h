#ifndef NET_H
#define NET_H

struct addrinfo *server_info;

int server_connect(const char *host, const char *port);
void net_cleanup();

#endif
