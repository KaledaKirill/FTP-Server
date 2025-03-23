#ifndef SAVE_SOCKET_H
#define SAVE_SOCKET_H

#include <sys/socket.h>

int socket_s(int domain, int type, int protocol);

int bind_s(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int listen_s(int sockfd, int backlog);

int accept_s(int sockfd, struct sockaddr *addr , socklen_t *addrlen);

#endif